#include "Aspen/Core/buffer.hpp"

namespace Aspen {
	void Buffer::makeBuffer(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, VertexMemory& vertexMemory) {
		createVertexBuffers(vertices, vertexMemory);
		createIndexBuffers(indices, vertexMemory);
	}

	void Buffer::createVertexBuffers(const std::vector<Vertex>& vertices, VertexMemory& vertexMemory) {
		// Get the number of bytes needed to store vertices in memory.
		uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

		// // Create the required buffers on the devide (GPU) memory.
		// // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = Allow CPU (Host) to access GPU (Device) memory in order to write to it.
		// // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = Keeps the Host memory regions and Device memory regions consistent with each other.
		// aspenDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

		// void *data = nullptr; // data will point to the beginning of the mapped memory range.
		// // Create a region of host memory which is mapped to device memory.
		// vkMapMemory(aspenDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);

		// // Copy the vertex buffer data into the host memory region.
		// // Note: Because we have set VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, this will automatically update the device memory region with the same data.
		// // Without this, we would have to manually call vkFlushMappedMemoryRanges().
		// memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		// vkUnmapMemory(aspenDevice.device(), vertexBufferMemory); // Unmap memory region (a.k.a free memory) when no longer needed.

		// Create staging buffer and allocate memory for it.
		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};
		aspenDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// TODO: Keep the staging buffer mapped and re-use the staging buffers.
		// Copy the vertices data into the staging buffer.
		void* stagingData = nullptr;
		vkMapMemory(aspenDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &stagingData);
		memcpy(stagingData, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(aspenDevice.device(), stagingBufferMemory);

		// Create a driver local vertex buffer.
		aspenDevice.createBuffer(
		    bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexMemory.vertexBuffer, vertexMemory.vertexBufferMemory);

		// Copy contents of staging buffer into device local vertex buffer.
		aspenDevice.copyBuffer(stagingBuffer, vertexMemory.vertexBuffer, bufferSize);

		// Clean up the staging buffer.
		vkDestroyBuffer(aspenDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(aspenDevice.device(), stagingBufferMemory, nullptr);
	}

	void Buffer::createIndexBuffers(const std::vector<uint16_t>& indices, VertexMemory& vertexMemory) {
		// Get the number of bytes needed to store indices in memory.
		uint32_t indexCount = static_cast<uint32_t>(indices.size());
		assert(indexCount >= 3 && "Index count must be at least 3");
		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

		// Create staging buffer and allocate memory for it.
		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};
		aspenDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// TODO: Keep the staging buffer mapped and re-use the staging buffers.
		// Copy the vertices data into the staging buffer.
		void* stagingData = nullptr;
		vkMapMemory(aspenDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &stagingData);
		memcpy(stagingData, indices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(aspenDevice.device(), stagingBufferMemory);

		// Create a driver local vertex buffer.
		aspenDevice.createBuffer(
		    bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexMemory.indexBuffer, vertexMemory.indexBufferMemory);

		// Copy contents of staging buffer into device local vertex buffer.
		aspenDevice.copyBuffer(stagingBuffer, vertexMemory.indexBuffer, bufferSize);

		// Clean up the staging buffer.
		vkDestroyBuffer(aspenDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(aspenDevice.device(), stagingBufferMemory, nullptr);
	}

	void Buffer::bind(VkCommandBuffer commandBuffer, VertexMemory& vertexMemory) {
		VkBuffer vertexBuffers[] = {vertexMemory.vertexBuffer};
		VkDeviceSize vertexOffsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vertexOffsets); // Record to command buffer to bind one vertex buffer starting at binding 0 (with offset of 0 into the buffer).
		vkCmdBindIndexBuffer(
		    commandBuffer, vertexMemory.indexBuffer, 0, VK_INDEX_TYPE_UINT16); // Record to command buffer to bind the index buffer starting at binding 0 (with offset of 0 into the buffer).
	}

	void Buffer::draw(VkCommandBuffer commandBuffer, const uint32_t count) const {
		// 1: Command buffer to render to.
		// 2: The number of vertices to draw.
		// 3: Instance count. Used for Instance rendering. Use 1 if instancing is not used.
		// 4: firstVertex - used as an offset into the vertex buffer. Defines the lowest value of gl_VertexIndex.
		// 5: firstInstance - used as an offset for instanced rendering. Defines the lowest valye of gl_InstanceIndex.
		// vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

		vkCmdDrawIndexed(commandBuffer, count, 1, 0, 0, 0);
	}

	// Defines how the vertex buffer is structured.
	std::vector<VkVertexInputBindingDescription> Buffer::Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex); // Interval from one vertex to the next in bytes.

		// VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex.
		// VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance.
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	// Defines the properties of each vertex inside the vertex buffer.
	std::vector<VkVertexInputAttributeDescription> Buffer::Vertex::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		attributeDescriptions[0].binding = 0;                         // The binding the attribute is located in.
		attributeDescriptions[0].location = 0;                        // The location value for input in the vertex shader.
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
		attributeDescriptions[0].offset = offsetof(Vertex, position); // Calculate the byte offset for the position attribute.

		attributeDescriptions[1].binding = 0;                         // The binding the attribute is located in.
		attributeDescriptions[1].location = 1;                        // The location value for input in the vertex shader.
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
		attributeDescriptions[1].offset = offsetof(Vertex, color);    // Calculate the byte offset for the color attribute.
		return attributeDescriptions;
	}
} // namespace Aspen