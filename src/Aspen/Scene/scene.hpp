#pragma once

#include "Aspen/Scene/components.hpp"
#include <entt/entt.hpp>

namespace Aspen {
	struct SceneData {
		std::unique_ptr<Buffer> vertexBuffer;
		std::unique_ptr<Buffer> indexBuffer;
		std::unique_ptr<Buffer> offsetBuffer;
		// std::unique_ptr<Buffer> textureBuffer;
		uint32_t textureCount = 0;
	};

	class Entity;
	class Scene {
	public:
		Scene(Device& device);
		~Scene();

		void updateSceneData();
		void updateTextures();

		Entity createEntity(const std::string& name = std::string());
		void OnUpdate();

		auto getRenderComponents() {
			return m_Registry.group<TransformComponent>(entt::get<MeshComponent>);
		};

		auto getMetaDataComponents() {
			return m_Registry.group<IDComponent>(entt::get<TagComponent>);
		};

		auto getPointLights() {
			return m_Registry.group<PointLightComponent>(entt::get<TransformComponent>);
		};

		const SceneData& getSceneData() const {
			return m_sceneData;
		}

		// Partial-owning group
		template <typename T, typename... Ts>
		decltype(auto) getComponents() {

			return m_Registry.group<T>(entt::get<Ts...>);
		}

	private:
		entt::registry m_Registry;
		Device& device;

		SceneData m_sceneData{};

		friend class Entity;
	};
} // namespace Aspen