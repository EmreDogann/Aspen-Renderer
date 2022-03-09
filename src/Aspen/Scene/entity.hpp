#pragma once
#include "Aspen/Scene/scene.hpp"

namespace Aspen {
	class Entity {
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);

		template <typename T, typename... Args>
		T& addComponent(Args&&... args) {
			assert(!hasComponent<T>() && "Entity already has component!");

			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template <typename... Ts>
		decltype(auto) getComponent() {
			assert(hasComponent<Ts...>() && "Entity does not have component!");

			return m_Scene->m_Registry.get<Ts...>(m_EntityHandle);
		}

		template <typename... Ts>
		bool hasComponent() {
			return m_Scene->m_Registry.all_of<Ts...>(m_EntityHandle);
		}

		template <typename T>
		void removeComponent() {
			assert(!hasComponent<T>() && "Entity already has component!");

			return m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		operator bool() const {
			return m_EntityHandle != entt::null;
		}

	private:
		entt::entity m_EntityHandle{entt::null};
		Scene* m_Scene;
	};
} // namespace Aspen