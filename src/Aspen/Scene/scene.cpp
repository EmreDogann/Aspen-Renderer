#include "Aspen/Scene/scene.hpp"
#include "entity.hpp"

namespace Aspen {
	Scene::Scene(Device& device)
	    : device(device) {
		// auto entity = createEntity();

		// m_Registry.emplace<TransformComponent>(entity);

		// if (m_Registry.all_of<TransformComponent>(entity)) {
		// 	TransformComponent &transform = m_Registry.get<TransformComponent>(entity);
		// }

		// Initialize groups by just calling them once.
		// m_Registry.group<TransformComponent, MeshComponent>();
		// m_Registry.group<PointLightComponent>(entt::get<TransformComponent>);
	}

	Scene::~Scene() {
		// Release all mesh component data from memory.
		// auto view = m_Registry.view<MeshComponent>();
		// for (auto entity : view) {
		// 	MeshComponent& mesh = view.get<MeshComponent>(entity);
		// 	// Clean up vertex buffer.
		// 	vkDestroyBuffer(device.device(), mesh.meshMemory.vertexBuffer, nullptr);
		// 	vkFreeMemory(device.device(), mesh.meshMemory.vertexBufferMemory, nullptr);

		// 	// Clean up index buffer.
		// 	vkDestroyBuffer(device.device(), mesh.meshMemory.indexBuffer, nullptr);
		// 	vkFreeMemory(device.device(), mesh.meshMemory.indexBufferMemory, nullptr);
		// }
	}

	Entity Scene::createEntity(const std::string& name) {
		Entity entity = {m_Registry.create(), this};

		entity.addComponent<IDComponent>();
		entity.addComponent<TransformComponent>();
		auto& tag = entity.addComponent<TagComponent>();
		tag.tag = name.empty() ? "Entity" : name;

		return entity;
	}

	void Scene::OnUpdate() {
		Camera* mainCamera = nullptr;

		// Find the main camera in the scene.
		auto view = m_Registry.view<TransformComponent, CameraComponent>();
		for (auto entity : view) {
			auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

			if (camera.primary) {
				mainCamera = &camera.camera;
				break;
			}
		}

		if (mainCamera) {}
	}
} // namespace Aspen