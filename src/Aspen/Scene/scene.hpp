#pragma once
#include "pch.h"

#include "Aspen/Scene/components.hpp"
#include <entt/entt.hpp>

namespace Aspen {
	class Entity;
	class Scene {
	public:
		Scene(AspenDevice& device);
		~Scene();

		Entity createEntity(const std::string& name = std::string());
		void OnUpdate();

		auto getRenderComponents() {
			return m_Registry.group<TransformComponent>(entt::get<MeshComponent>);
		};

	private:
		entt::registry m_Registry;
		AspenDevice& device;

		friend class Entity;
	};
} // namespace Aspen