#include "Aspen/Core/model.hpp"
#include <memory>

#define TINYOBJLOADER_IMPLEMENTATION    // Tells the pre-processor that this file will contain the implementation for TinyOBJLoader.
#define TINYOBJLOADER_USE_MAPBOX_EARCUT // Uses robust triangulation.
#include <tiny_obj_loader.h>

namespace std {
	// Takes an instance of the vertex struct and hashes it to a value of type size_t.
	template <>
	struct hash<Aspen::MeshComponent::Vertex> {
		size_t operator()(Aspen::MeshComponent::Vertex const& vertex) const {
			size_t seed = 0;
			Aspen::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
} // namespace std

namespace Aspen {
	void Model::makeBuffer(Device& device, MeshComponent& mesh) {
		createVertexBuffers(device, mesh.vertices, mesh.vertexBuffer);
		createIndexBuffers(device, mesh.indices, mesh.indexBuffer);
	}

	void Model::createVertexBuffers(Device& device, const std::vector<MeshComponent::Vertex>& vertices, std::unique_ptr<Buffer>& vertexBuffer) {
		// Get the number of bytes needed to store vertices in memory.
		uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");

		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);

		// // Create the required Buffers on the devide (GPU) memory.
		// // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = Allow CPU (Host) to access GPU (Device) memory in order to write to it.
		// // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = Keeps the Host memory regions and Device memory regions consistent with each other.
		// device.createBuffer(bufferSize, BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

		// void *data = nullptr; // data will point to the beginning of the mapped memory range.
		// // Create a region of host memory which is mapped to device memory.
		// vkMapMemory(device.device(), vertexBufferMemory, 0, bufferSize, 0, &data);

		// // Copy the vertex buffer data into the host memory region.
		// // Note: Because we have set VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, this will automatically update the device memory region with the same data.
		// // Without this, we would have to manually call vkFlushMappedMemoryRanges().
		// memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		// vkUnmapMemory(device.device(), vertexBufferMemory); // Unmap memory region (a.k.a free memory) when no longer needed.

		// Create staging buffer and allocate memory for it.
		Buffer stagingBuffer{
		    device,
		    vertexSize,
		    vertexCount,
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		// TODO: Keep the staging buffer mapped and re-use the staging buffers.
		// Copy the vertices data into the staging buffer.
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)vertices.data());

		// Create a device local vertex buffer.
		vertexBuffer = std::make_unique<Buffer>(
		    device,
		    vertexSize,
		    vertexCount,
		    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy contents of staging buffer into device local vertex buffer.
		device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}

	void Model::createIndexBuffers(Device& device, const std::vector<uint32_t>& indices, std::unique_ptr<Buffer>& indexBuffer) {
		// Get the number of bytes needed to store indices in memory.
		uint32_t indexCount = static_cast<uint32_t>(indices.size());
		assert(indexCount >= 3 && "Index count must be at least 3");

		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
		uint32_t indexSize = sizeof(indices[0]);

		// Create staging buffer and allocate memory for it.
		Buffer stagingBuffer{
		    device,
		    indexSize,
		    indexCount,
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		// TODO: Keep the staging buffer mapped and re-use the staging buffers.
		// Copy the index data into the staging buffer.
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());

		// Create a device local index buffer.
		indexBuffer = std::make_unique<Buffer>(
		    device,
		    indexSize,
		    indexCount,
		    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy contents of staging buffer into device local index buffer.
		device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
	}

	void Model::createModelFromFile(Device& device, MeshComponent& mesh, const std::string& filePath) {
		tinyobj::attrib_t attribute;          // Store position, color, normal, and texture coordinates.
		std::vector<tinyobj::shape_t> shapes; // Contains index values for each face.
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		// Load model specified from the file path.
		if (!tinyobj::LoadObj(&attribute, &shapes, &materials, &warn, &err, filePath.c_str())) {
			throw std::runtime_error(warn + err);
		}

		mesh.vertices.clear();
		mesh.indices.clear();

		// Will keep track of the vertices which have been added to the mesh component's vertices vector and store the position at which the vertex was originally added.
		std::unordered_map<MeshComponent::Vertex, uint32_t> uniqueVertices{};

		// Loop through each face in the model
		for (const auto& shape : shapes) {
			// Loop through each index in a face
			for (const auto& index : shape.mesh.indices) {
				MeshComponent::Vertex vertex{};

				// vertex_index is the first value of the face.
				// It tells you what position value to use. A negative index means no index was provided.

				// Read vertex positions and color
				if (index.vertex_index >= 0) {
					// Because we are traversing the attribute.vertices array in groups of 3, we need to multiply by 3.
					// The number added at the end corresponds to the position axis: 0 -> x, 1 -> y, 2 -> z.
					vertex.position = {
					    attribute.vertices[3 * index.vertex_index + 0], // x
					    attribute.vertices[3 * index.vertex_index + 1], // y
					    attribute.vertices[3 * index.vertex_index + 2], // z
					};

					vertex.color = {
					    attribute.colors[3 * index.vertex_index + 0], // x
					    attribute.colors[3 * index.vertex_index + 1], // y
					    attribute.colors[3 * index.vertex_index + 2], // z
					};
				}

				// Read vertex normals
				if (index.normal_index >= 0) {
					vertex.normal = {
					    attribute.normals[3 * index.normal_index + 0], // x
					    attribute.normals[3 * index.normal_index + 1], // y
					    attribute.normals[3 * index.normal_index + 2], // z
					};
				}

				// Read vertex texture coordinates
				if (index.texcoord_index >= 0) {
					vertex.uv = {
					    attribute.texcoords[2 * index.texcoord_index + 0], // u
					    attribute.texcoords[2 * index.texcoord_index + 1], // v
					};
				}

				// If the vertex is new, we add it to the uniqueVertices map.
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
					mesh.vertices.push_back(vertex);
				}
				mesh.indices.push_back(uniqueVertices[vertex]);
			}
		}

		// Allocate GPU local buffers and move the vertex and index data over to it.
		makeBuffer(device, mesh);
	}

	void Model::bind(VkCommandBuffer commandBuffer, std::unique_ptr<Buffer>& vertexBuffer, std::unique_ptr<Buffer>& indexBuffer) {
		VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
		VkDeviceSize vertexOffsets[] = {0};

		// Record to command buffer to bind one vertex buffer starting at binding 0 (with offset of 0 into the buffer).
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vertexOffsets);

		// Record to command buffer to bind the index buffer starting at binding 0 (with offset of 0 into the buffer).
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}

	void Model::draw(VkCommandBuffer commandBuffer, const uint32_t count) {
		// 1: Command buffer to render to.
		// 2: The number of vertices to draw.
		// 3: Instance count. Used for Instance rendering. Use 1 if instancing is not used.
		// 4: firstVertex - used as an offset into the vertex buffer. Defines the lowest value of gl_VertexIndex.
		// 5: firstInstance - used as an offset for instanced rendering. Defines the lowest valye of gl_InstanceIndex.
		// vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

		vkCmdDrawIndexed(commandBuffer, count, 1, 0, 0, 0);
	}

	// Defines how the vertex buffer is structured.
	std::vector<VkVertexInputBindingDescription> Model::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(MeshComponent::Vertex); // Interval from one vertex to the next in bytes.

		// VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex.
		// VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance.
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	// Defines the properties of each vertex inside the vertex buffer.
	std::vector<VkVertexInputAttributeDescription> Model::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		/* Parameters:
		    1 -> Location: The location value for input in the vertex shader.
		    2 -> Binding: The binding the attribute is located in.
		    3 -> Format: The number and type of components (e.g. a float vec3 would be VK_FORMAT_R32G32B32_SFLOAT),
		    4 -> Offset: The byte offset for the position attribute.
		*/
		attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshComponent::Vertex, position)});
		attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshComponent::Vertex, color)});
		attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MeshComponent::Vertex, normal)});
		attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(MeshComponent::Vertex, uv)});
		return attributeDescriptions;
	}
} // namespace Aspen