// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_INSTANCEDRENDERABLE_HPP
#define NAZARA_INSTANCEDRENDERABLE_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Graphics/Config.hpp>

namespace Nz
{
	class CommandBufferBuilder;
	class ModelInstance;

	class NAZARA_GRAPHICS_API InstancedRenderable
	{
		public:
			InstancedRenderable() = default;
			InstancedRenderable(const InstancedRenderable&) = delete;
			InstancedRenderable(InstancedRenderable&&) noexcept = default;
			~InstancedRenderable();

			virtual void Draw(CommandBufferBuilder& commandBuffer, ModelInstance& instance) const = 0;

			InstancedRenderable& operator=(const InstancedRenderable&) = delete;
			InstancedRenderable& operator=(InstancedRenderable&&) noexcept = default;
	};
}

#include <Nazara/Graphics/InstancedRenderable.inl>

#endif
