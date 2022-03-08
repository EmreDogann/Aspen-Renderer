#pragma once

#include "Aspen/Renderer/device.hpp"

// Libs & defines
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#include <glm/glm.hpp>

namespace Aspen {
	struct VertexMemory {
		VkBuffer vertexBuffer{};
		VkDeviceMemory vertexBufferMemory{};

		VkBuffer indexBuffer{};
		VkDeviceMemory indexBufferMemory{};
	};

	class Buffer {
	public:
		struct Vertex {
			glm::vec3 position;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		Buffer(AspenDevice& aspenDevice) : aspenDevice(aspenDevice){};
		~Buffer() = default;

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		Buffer(const Buffer&&) = delete;
		Buffer& operator=(const Buffer&&) = delete;

		void makeBuffer(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, VertexMemory& vertexMemory);

		void bind(VkCommandBuffer commandBuffer, VertexMemory& vertexMemory);
		void draw(VkCommandBuffer commandBuffer, const uint32_t count) const;

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices, VertexMemory& vertexMemory);
		void createIndexBuffers(const std::vector<uint16_t>& indices, VertexMemory& vertexMemory);

		AspenDevice& aspenDevice;
	};
} // namespace Aspen