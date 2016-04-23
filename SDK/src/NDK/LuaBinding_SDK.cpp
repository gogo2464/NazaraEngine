// Copyright (C) 2016 J�r�me Leclercq, Arnaud Cadot
// This file is part of the "Nazara Development Kit"
// For conditions of distribution and use, see copyright notice in Prerequesites.hpp

#include <NDK/LuaBinding.hpp>
#include <NDK/LuaAPI.hpp>

namespace Ndk
{
	void LuaBinding::BindSDK()
	{
		/*********************************** Ndk::Application **********************************/

		#ifndef NDK_SERVER
		//application.SetMethod("AddWindow", &Application::AddWindow);
		#endif
		application.BindMethod("AddWorld", [] (Nz::LuaInstance& instance, Application* application) -> int
		{
			instance.Push(application->AddWorld().CreateHandle());
			return 1;
		});

		application.BindMethod("GetUpdateTime", &Application::GetUpdateTime);
		application.BindMethod("Quit", &Application::Quit);

		/*********************************** Ndk::Console **********************************/
		#ifndef NDK_SERVER
		consoleClass.Inherit<Nz::Node>(nodeClass, [] (ConsoleHandle* handle) -> Nz::Node*
		{
			return handle->GetObject();
		});

		consoleClass.BindMethod("AddLine", &Console::AddLine, Nz::Color::White);
		consoleClass.BindMethod("Clear", &Console::Clear);
		consoleClass.BindMethod("GetCharacterSize", &Console::GetCharacterSize);
		consoleClass.BindMethod("GetHistory", &Console::GetHistory);
		consoleClass.BindMethod("GetHistoryBackground", &Console::GetHistoryBackground);
		consoleClass.BindMethod("GetInput", &Console::GetInput);
		consoleClass.BindMethod("GetInputBackground", &Console::GetInputBackground);
		consoleClass.BindMethod("GetSize", &Console::GetSize);
		consoleClass.BindMethod("GetTextFont", &Console::GetTextFont);

		consoleClass.BindMethod("IsVisible", &Console::IsVisible);

		consoleClass.BindMethod("SendCharacter", &Console::SendCharacter);
		//consoleClass.SetMethod("SendEvent", &Console::SendEvent);

		consoleClass.BindMethod("SetCharacterSize", &Console::SetCharacterSize);
		consoleClass.BindMethod("SetSize", &Console::SetSize);
		consoleClass.BindMethod("SetTextFont", &Console::SetTextFont);
		
		consoleClass.BindMethod("Show", &Console::Show, true);
		#endif

		/*********************************** Ndk::Entity **********************************/
		entityClass.BindMethod("Enable", &Entity::Enable);
		entityClass.BindMethod("GetId", &Entity::GetId);
		entityClass.BindMethod("GetWorld", &Entity::GetWorld);
		entityClass.BindMethod("Kill", &Entity::Kill);
		entityClass.BindMethod("IsEnabled", &Entity::IsEnabled);
		entityClass.BindMethod("IsValid", &Entity::IsValid);
		entityClass.BindMethod("RemoveComponent", (void(Entity::*)(ComponentIndex)) &Entity::RemoveComponent);
		entityClass.BindMethod("RemoveAllComponents", &Entity::RemoveAllComponents);
		entityClass.BindMethod("__tostring", &EntityHandle::ToString);

		entityClass.BindMethod("AddComponent", [this] (Nz::LuaInstance& lua, EntityHandle& handle) -> int
		{
			int index = 1;
			ComponentIndex componentIndex = lua.Check<ComponentIndex>(&index);

			if (componentIndex > m_componentBinding.size())
			{
				lua.Error("Invalid component index");
				return 0;
			}

			ComponentBinding& binding = m_componentBinding[componentIndex];
			if (binding.name.IsEmpty())
			{
				lua.Error("This component is not available to the LuaAPI");
				return 0;
			}

			return binding.adder(lua, handle);
		});

		entityClass.BindMethod("GetComponent", [this] (Nz::LuaInstance& lua, EntityHandle& handle) -> int
		{
			int index = 1;
			ComponentIndex componentIndex = lua.Check<ComponentIndex>(&index);

			if (!handle->HasComponent(componentIndex))
			{
				lua.PushNil();
				return 1;
			}

			if (componentIndex > m_componentBinding.size())
			{
				lua.Error("Invalid component index");
				return 0;
			}

			ComponentBinding& binding = m_componentBinding[componentIndex];
			if (binding.name.IsEmpty())
			{
				lua.Error("This component is not available to the LuaAPI");
				return 0;
			}

			return binding.getter(lua, handle->GetComponent(componentIndex));
		});

		/*********************************** Ndk::NodeComponent **********************************/
		nodeComponent.Inherit<Nz::Node>(nodeClass, [] (NodeComponentHandle* handle) -> Nz::Node*
		{
			return handle->GetObject();
		});

		/*********************************** Ndk::VelocityComponent **********************************/
		velocityComponent.SetGetter([] (Nz::LuaInstance& lua, VelocityComponentHandle& instance)
		{
			std::size_t length;
			const char* member = lua.CheckString(1, &length);

			if (std::strcmp(member, "Linear") == 0)
			{
				lua.Push(instance->linearVelocity);
				return true;
			}

			return false;
		});

		velocityComponent.SetSetter([] (Nz::LuaInstance& lua, VelocityComponentHandle& instance)
		{
			std::size_t length;
			const char* member = lua.CheckString(1, &length);

			int argIndex = 2;
			if (std::strcmp(member, "Linear") == 0)
			{
				instance->linearVelocity = lua.Check<Nz::Vector3f>(&argIndex);
				return true;
			}

			return false;
		});

		/*********************************** Ndk::World **********************************/
		worldClass.BindMethod("CreateEntity", &World::CreateEntity);
		worldClass.BindMethod("CreateEntities", &World::CreateEntities);
		worldClass.BindMethod("Clear", &World::Clear);


		#ifndef NDK_SERVER
		/*********************************** Ndk::GraphicsComponent **********************************/
		graphicsComponent.BindMethod("Attach", &GraphicsComponent::Attach, 0);
		#endif


		// Components functions
		m_componentBinding.resize(BaseComponent::GetMaxComponentIndex());

		BindComponent<NodeComponent>("Node");
		BindComponent<VelocityComponent>("Velocity");

		#ifndef NDK_SERVER
		BindComponent<GraphicsComponent>("Graphics");
		#endif
	}

	void LuaBinding::RegisterSDK(Nz::LuaInstance& instance)
	{
		// Classes
		application.Register(instance);
		entityClass.Register(instance);
		nodeComponent.Register(instance);
		velocityComponent.Register(instance);
		worldClass.Register(instance);

		#ifndef NDK_SERVER
		consoleClass.Register(instance);
		graphicsComponent.Register(instance);
		#endif

		// Enums

		// ComponentType (fake enumeration to expose component indexes)
		instance.PushTable(0, m_componentBinding.size());
		{
			for (const ComponentBinding& entry : m_componentBinding)
			{
				if (entry.name.IsEmpty())
					continue;

				instance.PushField(entry.name, entry.index);
			}
		}
		instance.SetGlobal("ComponentType");
	}
}
