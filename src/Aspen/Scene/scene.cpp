#include "Aspen/Scene/scene.hpp"

#include "pch.h"
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
		m_sceneData.vertexBuffer.reset();
		m_sceneData.indexBuffer.reset();
		m_sceneData.offsetBuffer.reset();
		m_sceneData.materialBuffer.reset();
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
		std::vector<MaterialComponent> materials;
		std::vector<glm::uvec2> offsets;

		auto group = getRenderComponents();
		for (const auto& entity : group) {
			auto [mesh, meshMaterial] = group.get<MeshComponent, MaterialComponent>(entity);

			// Remember the index, vertex offsets.
			const auto indexOffset = static_cast<uint32_t>(indices.size());
			const auto vertexOffset = static_cast<uint32_t>(vertices.size());
			const auto materialOffset = static_cast<uint32_t>(materials.size());

			offsets.emplace_back(indexOffset, vertexOffset);

			// Copy model data one after the other.
			vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
			indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());
			materials.push_back(meshMaterial);

			// Adjust the texture id.
			for (size_t i = vertexOffset; i != vertices.size(); ++i) {
				vertices[i].materialIndex = materialOffset;
			}
		}

		Model::createVertexBuffers(device, vertices, m_sceneData.vertexBuffer);
		Model::createIndexBuffers(device, indices, m_sceneData.indexBuffer);

		// Upload Offsets array to GPU memory.
		{
			// Create staging buffer and allocate memory for it.
			Buffer stagingBuffer{
			    device,
			    sizeof(glm::uvec2),
			    static_cast<uint32_t>(offsets.size()),
			    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			};

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

			device.copyBuffer(stagingBuffer.getBuffer(), m_sceneData.offsetBuffer->getBuffer(), sizeof(glm::uvec2) * static_cast<uint32_t>(offsets.size()));
		}

		// Upload materials array to GPU memory.
		{
			Buffer stagingBuffer{
			    device,
			    sizeof(MaterialComponent),
			    static_cast<uint32_t>(materials.size()),
			    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			};
			stagingBuffer.map();
			stagingBuffer.writeToBuffer((void*)materials.data());

			m_sceneData.materialBuffer = std::make_unique<Buffer>(
			    device,
			    sizeof(MaterialComponent),
			    static_cast<uint32_t>(materials.size()),
			    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			device.copyBuffer(stagingBuffer.getBuffer(), m_sceneData.materialBuffer->getBuffer(), sizeof(MaterialComponent) * static_cast<uint32_t>(materials.size()));
		}
	}

	int32_t Scene::addTexture(Texture2D& textureHandle, std::string relativeFilepath, VkFormat format, VkQueue copyQueue) {
		textureHandle.loadFromFile(&device, std::move(relativeFilepath), format, copyQueue);

		if (textureHandle.isTextureLoaded) {
			return m_sceneData.textureCount++;
		}

		std::cout << "Loading of texture: " << relativeFilepath << " was not successful!" << std::endl;
		return -1;
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