// Copyright (C) 2022 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Graphics/ForwardPipelinePass.hpp>
#include <Nazara/Graphics/AbstractViewer.hpp>
#include <Nazara/Graphics/FrameGraph.hpp>
#include <Nazara/Graphics/FramePipeline.hpp>
#include <Nazara/Graphics/InstancedRenderable.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/PredefinedShaderStructs.hpp>
#include <Nazara/Graphics/ViewerInstance.hpp>
#include <Nazara/Renderer/CommandBufferBuilder.hpp>
#include <Nazara/Renderer/RenderFrame.hpp>
#include <Nazara/Graphics/Debug.hpp>

namespace Nz
{
	ForwardPipelinePass::ForwardPipelinePass(FramePipeline& owner, AbstractViewer* viewer) :
	m_lastVisibilityHash(0),
	m_viewer(viewer),
	m_pipeline(owner),
	m_rebuildCommandBuffer(false),
	m_rebuildElements(false)
	{
		Graphics* graphics = Graphics::Instance();
		m_forwardPassIndex = graphics->GetMaterialPassRegistry().GetPassIndex("ForwardPass");
		m_lightUboPool = std::make_shared<LightUboPool>();
	}

	ForwardPipelinePass::~ForwardPipelinePass()
	{
		for (auto&& [materialPass, entry] : m_materialPasses)
			m_pipeline.UnregisterMaterialPass(materialPass);
	}

	void ForwardPipelinePass::Prepare(RenderFrame& renderFrame, const Frustumf& frustum, const std::vector<FramePipelinePass::VisibleRenderable>& visibleRenderables, const std::vector<const Light*>& visibleLights, std::size_t visibilityHash)
	{
		if (m_lastVisibilityHash != visibilityHash || m_rebuildElements) //< FIXME
		{
			renderFrame.PushForRelease(std::move(m_renderElements));
			m_renderElements.clear();
			m_renderQueueRegistry.Clear();
			m_renderQueue.Clear();
			m_lightBufferPerLights.clear();
			m_lightPerRenderElement.clear();

			for (auto& lightDataUbo : m_lightDataBuffers)
			{
				renderFrame.PushReleaseCallback([pool = m_lightUboPool, lightUbo = std::move(lightDataUbo.renderBuffer)]()
				{
					pool->lightUboBuffers.push_back(std::move(lightUbo));
				});
			}
			m_lightDataBuffers.clear();

			Graphics* graphics = Graphics::Instance();

			PredefinedLightData lightOffsets = PredefinedLightData::GetOffsets();
			std::size_t lightUboAlignedSize = AlignPow2(lightOffsets.totalSize, graphics->GetRenderDevice()->GetDeviceInfo().limits.minUniformBufferOffsetAlignment);

			UploadPool& uploadPool = renderFrame.GetUploadPool();

			for (const auto& renderableData : visibleRenderables)
			{
				BoundingVolumef renderableBoundingVolume(renderableData.instancedRenderable->GetAABB());
				renderableBoundingVolume.Update(renderableData.worldInstance->GetWorldMatrix());

				// Select lights
				m_renderableLights.clear();
				for (const Light* light : visibleLights)
				{
					const BoundingVolumef& boundingVolume = light->GetBoundingVolume();
					if (boundingVolume.Intersect(renderableBoundingVolume.aabb))
						m_renderableLights.push_back(light);
				}

				// Sort lights
				std::sort(m_renderableLights.begin(), m_renderableLights.end(), [&](const Light* lhs, const Light* rhs)
				{
					return lhs->ComputeContributionScore(renderableBoundingVolume) < rhs->ComputeContributionScore(renderableBoundingVolume);
				});

				std::size_t lightCount = std::min(m_renderableLights.size(), MaxLightCountPerDraw);

				LightKey lightKey;
				lightKey.fill(nullptr);
				for (std::size_t i = 0; i < lightCount; ++i)
					lightKey[i] = m_renderableLights[i];

				RenderBufferView lightUboView;

				auto it = m_lightBufferPerLights.find(lightKey);
				if (it == m_lightBufferPerLights.end())
				{
					// Prepare light ubo upload

					// Find light ubo
					LightDataUbo* targetLightData = nullptr;
					for (auto& lightUboData : m_lightDataBuffers)
					{
						if (lightUboData.offset + lightUboAlignedSize <= lightUboData.renderBuffer->GetSize())
						{
							targetLightData = &lightUboData;
							break;
						}
					}

					if (!targetLightData)
					{
						// Make a new light UBO
						auto& lightUboData = m_lightDataBuffers.emplace_back();

						// Reuse from pool if possible
						if (!m_lightUboPool->lightUboBuffers.empty())
						{
							lightUboData.renderBuffer = m_lightUboPool->lightUboBuffers.back();
							m_lightUboPool->lightUboBuffers.pop_back();
						}
						else
							lightUboData.renderBuffer = graphics->GetRenderDevice()->InstantiateBuffer(BufferType::Uniform, 256 * lightUboAlignedSize, BufferUsage::DeviceLocal | BufferUsage::Dynamic | BufferUsage::Write);

						targetLightData = &lightUboData;
					}

					assert(targetLightData);
					if (!targetLightData->allocation)
						targetLightData->allocation = &uploadPool.Allocate(targetLightData->renderBuffer->GetSize());

					void* lightDataPtr = static_cast<UInt8*>(targetLightData->allocation->mappedPtr) + targetLightData->offset;
					AccessByOffset<UInt32&>(lightDataPtr, lightOffsets.lightCountOffset) = SafeCast<UInt32>(lightCount);

					UInt8* lightPtr = static_cast<UInt8*>(lightDataPtr) + lightOffsets.lightsOffset;
					for (std::size_t i = 0; i < lightCount; ++i)
					{
						m_renderableLights[i]->FillLightData(lightPtr);
						lightPtr += lightOffsets.lightSize;
					}

					// Associate render element with light ubo
					lightUboView = RenderBufferView(targetLightData->renderBuffer.get(), targetLightData->offset, lightUboAlignedSize);

					targetLightData->offset += lightUboAlignedSize;

					m_lightBufferPerLights.emplace(lightKey, lightUboView);
				}
				else
					lightUboView = it->second;

				std::size_t previousCount = m_renderElements.size();
				renderableData.instancedRenderable->BuildElement(m_forwardPassIndex, *renderableData.worldInstance, m_renderElements);
				for (std::size_t i = previousCount; i < m_renderElements.size(); ++i)
				{
					const RenderElement* element = m_renderElements[i].get();
					m_lightPerRenderElement.emplace(element, lightUboView);
				}
			}

			for (const auto& renderElement : m_renderElements)
			{
				renderElement->Register(m_renderQueueRegistry);
				m_renderQueue.Insert(renderElement.get());
			}

			m_renderQueueRegistry.Finalize();

			renderFrame.Execute([&](CommandBufferBuilder& builder)
			{
				builder.BeginDebugRegion("Light UBO Update", Color::Yellow);
				{
					for (auto& lightUboData : m_lightDataBuffers)
					{
						if (!lightUboData.allocation)
							continue;

						builder.CopyBuffer(*lightUboData.allocation, RenderBufferView(lightUboData.renderBuffer.get(), 0, lightUboData.offset));
					}

					builder.PostTransferBarrier();
				}
				builder.EndDebugRegion();
			}, QueueType::Transfer);

			m_lastVisibilityHash = visibilityHash;
			m_rebuildElements = true;
		}

		// TODO: Don't sort every frame if no material pass requires distance sorting
		m_renderQueue.Sort([&](const RenderElement* element)
		{
			return element->ComputeSortingScore(frustum, m_renderQueueRegistry);
		});

		if (m_rebuildElements)
		{
			m_pipeline.ForEachElementRenderer([&](std::size_t elementType, ElementRenderer& elementRenderer)
			{
				if (elementType >= m_elementRendererData.size() || !m_elementRendererData[elementType])
				{
					if (elementType >= m_elementRendererData.size())
						m_elementRendererData.resize(elementType + 1);

					m_elementRendererData[elementType] = elementRenderer.InstanciateData();
				}

				elementRenderer.Reset(*m_elementRendererData[elementType], renderFrame);
			});

			const auto& viewerInstance = m_viewer->GetViewerInstance();

			auto& lightPerRenderElement = m_lightPerRenderElement;
			m_pipeline.ProcessRenderQueue(m_renderQueue, [&](std::size_t elementType, const Pointer<const RenderElement>* elements, std::size_t elementCount)
			{
				ElementRenderer& elementRenderer = m_pipeline.GetElementRenderer(elementType);

				m_renderStates.clear();

				m_renderStates.reserve(elementCount);
				for (std::size_t i = 0; i < elementCount; ++i)
				{
					auto it = lightPerRenderElement.find(elements[i]);
					assert(it != lightPerRenderElement.end());

					auto& renderStates = m_renderStates.emplace_back();
					renderStates.lightData = it->second;
				}

				elementRenderer.Prepare(viewerInstance, *m_elementRendererData[elementType], renderFrame, elementCount, elements, m_renderStates.data());
			});

			m_pipeline.ForEachElementRenderer([&](std::size_t elementType, ElementRenderer& elementRenderer)
			{
				elementRenderer.PrepareEnd(renderFrame, *m_elementRendererData[elementType]);
			});

			m_rebuildCommandBuffer = true;
			m_rebuildElements = false;
		}
	}

	void ForwardPipelinePass::RegisterMaterial(const Material& material)
	{
		if (!material.HasPass(m_forwardPassIndex))
			return;

		MaterialPass* materialPass = material.GetPass(m_forwardPassIndex).get();

		auto it = m_materialPasses.find(materialPass);
		if (it == m_materialPasses.end())
		{
			m_pipeline.RegisterMaterialPass(materialPass);

			auto& matPassEntry = m_materialPasses[materialPass];
			matPassEntry.onMaterialPipelineInvalidated.Connect(materialPass->OnMaterialPassPipelineInvalidated, [=](const MaterialPass*)
			{
				m_rebuildElements = true;
			});

			matPassEntry.onMaterialShaderBindingInvalidated.Connect(materialPass->OnMaterialPassShaderBindingInvalidated, [=](const MaterialPass*)
			{
				m_rebuildCommandBuffer = true;
			});
		}
		else
			it->second.usedCount++;
	}

	void ForwardPipelinePass::RegisterToFrameGraph(FrameGraph& frameGraph, std::size_t colorBufferIndex, std::size_t depthBufferIndex)
	{
		FramePass& forwardPass = frameGraph.AddPass("Forward pass");
		forwardPass.AddOutput(colorBufferIndex);
		forwardPass.SetDepthStencilInput(depthBufferIndex);

		forwardPass.SetClearColor(0, m_viewer->GetClearColor());
		forwardPass.SetDepthStencilClear(1.f, 0);

		forwardPass.SetExecutionCallback([&]()
		{
			if (m_rebuildCommandBuffer)
				return FramePassExecution::UpdateAndExecute;
			else
				return FramePassExecution::Execute;
		});

		forwardPass.SetCommandCallback([this](CommandBufferBuilder& builder, const Recti& /*renderRect*/)
		{
			Recti viewport = m_viewer->GetViewport();

			builder.SetScissor(viewport);
			builder.SetViewport(viewport);

			const auto& viewerInstance = m_viewer->GetViewerInstance();

			m_pipeline.ProcessRenderQueue(m_renderQueue, [&](std::size_t elementType, const Pointer<const RenderElement>* elements, std::size_t elementCount)
			{
				ElementRenderer& elementRenderer = m_pipeline.GetElementRenderer(elementType);
				elementRenderer.Render(viewerInstance, *m_elementRendererData[elementType], builder, elementCount, elements);
			});

			m_rebuildCommandBuffer = false;
		});
	}

	void ForwardPipelinePass::UnregisterMaterial(const Material& material)
	{
		if (!material.HasPass(m_forwardPassIndex))
			return;

		MaterialPass* materialPass = material.GetPass(m_forwardPassIndex).get();

		auto it = m_materialPasses.find(materialPass);
		if (it != m_materialPasses.end())
		{
			if (--it->second.usedCount == 0)
			{
				m_materialPasses.erase(it);
				m_pipeline.UnregisterMaterialPass(materialPass);
			}
		}
	}
}
