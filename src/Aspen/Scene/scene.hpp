#pragma once

#include "Aspen/Renderer/device.hpp"
#include "Aspen/Scene/components.hpp"
#include <entt/entt.hpp>

namespace Aspen {
	class Entity;
	class Scene {
	public:
		Scene(Device& device);
		~Scene();

		Entity createEntity(const std::string& name = std::string());
		void OnUpdate();

		auto getRenderComponents() {
			return m_Registry.group<TransformComponent>(entt::get<MeshComponent>);
		};

		auto getPointLights() {
			return m_Registry.group<PointLightComponent>(entt::get<TransformComponent>);
		};

		// Partial-owning group
		template <typename T, typename... Ts>
		decltype(auto) getComponents() {

			return m_Registry.group<T>(entt::get<Ts...>);
		}

	private:
		entt::registry m_Registry;
		Device& device;

		friend class Entity;
	};
} // namespace Aspen