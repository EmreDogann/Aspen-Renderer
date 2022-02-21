#pragma once

#include "aspen_device.hpp"

// Libs & defines
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

// std
#include <cassert>
#include <cstring>
#include <vector>

namespace Aspen {
	class AspenModel {
	public:
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		AspenModel(AspenDevice &device, const std::vector<Vertex> &vertices);
		~AspenModel();

		AspenModel(const AspenModel &) = delete;
		AspenModel &operator=(const AspenModel &) = delete;

		AspenModel(const AspenModel &&) = delete;
		AspenModel &operator=(const AspenModel &&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer) const;

	private:
		void createVertexBuffers(const std::vector<Vertex> &vertices);

		AspenDevice &aspenDevice;
		VkBuffer vertexBuffer{};
		VkDeviceMemory vertexBufferMemory{};
		uint32_t vertexCount = 0;
	};
} // namespace Aspen