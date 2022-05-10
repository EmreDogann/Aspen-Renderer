#include "Aspen/Scene/scene.hpp"
#include "entity.hpp"
#include "Aspen/Core/model.hpp"

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
		m_sceneData.vertexBuffer.reset();
		m_sceneData.indexBuffer.reset();
		m_sceneData.offsetBuffer.reset();
	}

	Entity Scene::createEntity(const std::string& name) {
		Entity entity = {m_Registry.create(), this};

		entity.addComponent<IDComponent>();
		entity.addComponent<TransformComponent>();
		auto& tag = entity.addComponent<TagComponent>();
		tag.tag = name.empty() ? "Entity" : name;

		return entity;
	}

	void Scene::updateSceneData() {
		// Concatenate all the models
		std::vector<MeshComponent::Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<int32_t> textures;
		std::vector<glm::uvec2> offsets;

		int textureIndex = 0;

		auto group = getRenderComponents();
		for (const auto& entity : group) {
			auto& mesh = group.get<MeshComponent>(entity);

			// Remember the index, vertex offsets.
			const auto indexOffset = static_cast<uint32_t>(indices.size());
			const auto vertexOffset = static_cast<uint32_t>(vertices.size());
			const auto textureOffset = static_cast<int32_t>(textures.size());

			offsets.emplace_back(indexOffset, vertexOffset);

			// Copy model data one after the other.
			vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
			indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());

			if (mesh.texture.isTextureLoaded) {
				textures.push_back(textureIndex++);
			} else {
				textures.push_back(-1);
			}

			// Adjust the texture id.
			for (size_t i = vertexOffset; i != vertices.size(); ++i) {
				vertices[i].textureIndex = textureOffset;
			}
		}

		Model::createVertexBuffers(device, vertices, m_sceneData.vertexBuffer);
		Model::createIndexBuffers(device, indices, m_sceneData.indexBuffer);

		// Create staging buffer and allocate memory for it.
		Buffer stagingBuffer{
		    device,
		    sizeof(glm::uvec2),
		    static_cast<uint32_t>(offsets.size()),
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		// TODO: Keep the staging buffer mapped and re-use the staging buffers.
		// Copy the vertices data into the staging buffer.
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)offsets.data());

		// Create a device local buffer.
		// VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT - Allows us to pass the device address of the buffer to the acceleration structure used for ray tracing.
		m_sceneData.offsetBuffer = std::make_unique<Buffer>(
		    device,
		    sizeof(glm::uvec2),
		    static_cast<uint32_t>(offsets.size()),
		    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy contents of staging buffer into device local buffer.
		device.copyBuffer(stagingBuffer.getBuffer(), m_sceneData.offsetBuffer->getBuffer(), sizeof(glm::uvec2) * static_cast<uint32_t>(offsets.size()));

		stagingBuffer.writeToBuffer((void*)textures.data());

		// Create a device local buffer.
		// VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT - Allows us to pass the device address of the buffer to the acceleration structure used for ray tracing.
		m_sceneData.textureIDBuffer = std::make_unique<Buffer>(
		    device,
		    sizeof(int32_t),
		    static_cast<uint32_t>(textures.size()),
		    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy contents of staging buffer into device local buffer.
		device.copyBuffer(stagingBuffer.getBuffer(), m_sceneData.textureIDBuffer->getBuffer(), sizeof(glm::int32_t) * static_cast<uint32_t>(textures.size()));
	}

	void Scene::updateTextures() {
		auto group = getRenderComponents();
		for (const auto& entity : group) {
			auto& mesh = group.get<MeshComponent>(entity);

			if (mesh.texture.isTextureLoaded) {
				m_sceneData.textureCount++;
			}
		}
	}

	void Scene::OnUpdate() {
		// Camera* mainCamera = nullptr;

		// // Find the main camera in the scene.
		// auto view = m_Registry.view<TransformComponent, CameraComponent>();
		// for (auto entity : view) {
		// 	auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

		// 	if (camera.primary) {
		// 		mainCamera = &camera.camera;
		// 		break;
		// 	}
		// }

		// if (mainCamera) {}
	}
} // namespace Aspen