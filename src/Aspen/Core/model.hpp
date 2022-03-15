#pragma once
#include "pch.h"

#include "Aspen/Renderer/buffer.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Scene/components.hpp"
#include "Aspen/Utils/utils.hpp"

// Libs & defines
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace Aspen {
	class Model {
	public:
		Model() = default;
		~Model() = default;

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		Model(const Model&&) = delete;
		Model& operator=(const Model&&) = delete;

		static void makeBuffer(Device& device, MeshComponent& mesh);
		static void createModelFromFile(Device& device, MeshComponent& mesh, const std::string& filePath);

		static void bind(VkCommandBuffer commandBuffer, std::unique_ptr<Buffer>& vertexBuffer, std::unique_ptr<Buffer>& indexBuffer);
		static void draw(VkCommandBuffer commandBuffer, const uint32_t count);

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	private:
		static void createVertexBuffers(Device& device, const std::vector<MeshComponent::Vertex>& vertices, std::unique_ptr<Buffer>& vertexBuffer);
		static void createIndexBuffers(Device& device, const std::vector<uint32_t>& indices, std::unique_ptr<Buffer>& indexBuffer);
	};
} // namespace Aspen