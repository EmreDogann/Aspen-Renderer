#include "Aspen/Scene/scene.hpp"
#include "entity.hpp"

namespace Aspen {
	Scene::Scene(AspenDevice& device) : device(device) {
		// auto entity = createEntity();

		// m_Registry.emplace<TransformComponent>(entity);

		// if (m_Registry.all_of<TransformComponent>(entity)) {
		// 	TransformComponent &transform = m_Registry.get<TransformComponent>(entity);
		// }
	}

	Scene::~Scene() {
		// Release all mesh component data from memory.
		auto view = m_Registry.view<MeshComponent>();
		for (auto entity : view) {
			MeshComponent& mesh = view.get<MeshComponent>(entity);
			// Clean up vertex buffer.
			vkDestroyBuffer(device.device(), mesh.vertexMemory.vertexBuffer, nullptr);
			vkFreeMemory(device.device(), mesh.vertexMemory.vertexBufferMemory, nullptr);

			// Clean up index buffer.
			vkDestroyBuffer(device.device(), mesh.vertexMemory.indexBuffer, nullptr);
			vkFreeMemory(device.device(), mesh.vertexMemory.indexBufferMemory, nullptr);
		}
	}

	Entity Scene::createEntity(const std::string& name) {
		Entity entity = {m_Registry.create(), this};

		entity.addComponent<TransformComponent>();
		auto& tag = entity.addComponent<TagComponent>();
		tag.tag = name.empty() ? "Entity" : name;

		return entity;
	}

	void Scene::OnUpdate() {
		AspenCamera* mainCamera = nullptr;

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