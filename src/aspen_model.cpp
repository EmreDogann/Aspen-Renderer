#include "aspen_model.hpp"

// std
#include <cassert>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace Aspen {
    AspenModel::AspenModel(AspenDevice &device, const std::vector<Vertex> &vertices) : aspenDevice(device) {
        createVertexBuffers(vertices);
    }

    AspenModel::~AspenModel() {
        vkDestroyBuffer(aspenDevice.device(), vertexBuffer, nullptr);
        vkFreeMemory(aspenDevice.device(), vertexBufferMemory, nullptr);
    }

    void AspenModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
        // Get the number of bytes needed to store vertices in memory.
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

        // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = Allow CPU (Host) to access GPU (Device) memory in order to write to it.
        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = Keeps the Host memory regions and Device memory regions consistent with each other.
        aspenDevice.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

        void *data; // data will point to the beginning of the mapped memory range.
        // Create a region of host memory which is mapped to device memory.
        vkMapMemory(aspenDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);

        // Copy the vertex buffer data into the host memory region.
        // Note: Because we have set VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this will automatically update the device memory region with the same data.
        // Without this, we would have to manually call vkFlushMappedMemoryRanges().
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(aspenDevice.device(), vertexBufferMemory); // Unmap memory region (a.k.a free memory) when no longer needed.
    }

    void AspenModel::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    }

    void AspenModel::draw(VkCommandBuffer commandBuffer) {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }

    std::vector<VkVertexInputBindingDescription> AspenModel::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> AspenModel::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = 0;
        return attributeDescriptions;
    }
}