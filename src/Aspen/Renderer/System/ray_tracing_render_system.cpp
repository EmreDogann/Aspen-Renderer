#include "Aspen/Renderer/System/ray_tracing_render_system.hpp"

namespace Aspen {
	RayTracingRenderSystem::RayTracingRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayouts, std::shared_ptr<Framebuffer> resourcesDepthPrePass)
	    : device(device), deviceProcedures(device.deviceProcedures()), renderer(renderer), resources(std::make_unique<Framebuffer>(device)), resourcesDepthPrePass(resourcesDepthPrePass), globalDescriptorSetLayouts(globalDescriptorSetLayouts), textureDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT) {
		createResources();
	}

	RayTracingRenderSystem::~RayTracingRenderSystem() {
		vkDestroyImageView(device.device(), storage_image.view, nullptr);
		vkDestroyImage(device.device(), storage_image.image, nullptr);
		vkFreeMemory(device.device(), storage_image.memory, nullptr);

		for (auto& blas : m_BLAS) {
			blas.buffer.reset();
			deviceProcedures.vkDestroyAccelerationStructureKHR(device.device(), blas.handle, nullptr);
		}
		m_TLAS.buffer.reset();
		deviceProcedures.vkDestroyAccelerationStructureKHR(device.device(), m_TLAS.handle, nullptr);
	}

	void RayTracingRenderSystem::createResources() {

		storage_image.width = renderer.getSwapChainExtent().width;
		storage_image.height = renderer.getSwapChainExtent().height;
		storage_image.format = VK_FORMAT_B8G8R8A8_UNORM; // TODO: This Image needs to be copied to a SRGB image before displaying.

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = storage_image.format;
		imageInfo.extent.width = storage_image.width;
		imageInfo.extent.height = storage_image.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		// VK_IMAGE_USAGE_TRANSFER_SRC_BIT - So image is used as a source of a copy function.
		// VK_IMAGE_USAGE_STORAGE_BIT - So the shaders can read and write to the image.
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, storage_image.image, storage_image.memory);

		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = storage_image.format;
		viewCreateInfo.subresourceRange = {};
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;
		viewCreateInfo.image = storage_image.image;
		VK_CHECK(vkCreateImageView(device.device(), &viewCreateInfo, nullptr, &storage_image.view));

		VkCommandBuffer commandBuffer = device.beginSingleTimeCommandBuffers();

		VulkanTools::setImageLayout(
		    commandBuffer,
		    storage_image.image,
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    VK_IMAGE_LAYOUT_GENERAL,
		    viewCreateInfo.subresourceRange);

		device.endSingleTimeCommandBuffers(commandBuffer);

		// Color Attachment
		{
			AttachmentCreateInfo attachmentCreateInfo{};
			attachmentCreateInfo.format = renderer.getSwapChainImageFormat();
			attachmentCreateInfo.width = renderer.getSwapChainExtent().width;
			attachmentCreateInfo.height = renderer.getSwapChainExtent().height;
			attachmentCreateInfo.imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
			attachmentCreateInfo.layerCount = 1;
			attachmentCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			attachmentCreateInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentCreateInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentCreateInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentCreateInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentCreateInfo.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			resources->createAttachment(attachmentCreateInfo);
		}

		// Depth Attachment
		{
			std::shared_ptr<Framebuffer> tempFramebuffer = resourcesDepthPrePass.lock();

			AttachmentAddInfo attachmentAddInfo{};
			VulkanTools::getSupportedDepthFormat(device.physicalDevice(), &attachmentAddInfo.format);
			attachmentAddInfo.imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
			attachmentAddInfo.view = tempFramebuffer->attachments[0].view;
			attachmentAddInfo.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			attachmentAddInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			attachmentAddInfo.layerCount = 1;
			attachmentAddInfo.width = renderer.getSwapChainExtent().width;
			attachmentAddInfo.height = renderer.getSwapChainExtent().height;
			attachmentAddInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentAddInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentAddInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentAddInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

			resources->addLoadAttachment(attachmentAddInfo);
		}

		resources->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		resources->createRenderPass();
	}

	void RayTracingRenderSystem::updateResources() {
		// Recreate the storage image.
		vkDestroyImageView(device.device(), storage_image.view, nullptr);
		vkDestroyImage(device.device(), storage_image.image, nullptr);
		vkFreeMemory(device.device(), storage_image.memory, nullptr);

		createResources();

		// Update the descriptor set binding.
		VkDescriptorImageInfo image_descriptor{};
		image_descriptor.imageView = storage_image.view;
		image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		DescriptorWriter(*rtDescriptorSetLayout, device.getDescriptorPool())
		    .writeImage(1, &image_descriptor)
		    .overwrite(rtDescriptorSet);
	}

	/*
	    Copy ray tracing output to offscreen image
	*/
	void RayTracingRenderSystem::copyToImage(VkCommandBuffer cmdBuffer, VkImage dstImage, VkImageLayout initialLayout, uint32_t width, uint32_t height) {
		// Wait for destination image to finish transitioning.
		VulkanTools::setImageLayout(
		    cmdBuffer,
		    dstImage,
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    initialLayout,
		    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

		// Prepare destination image as transfer destination
		VulkanTools::setImageLayout(
		    cmdBuffer,
		    dstImage,
		    initialLayout,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

		// Prepare ray tracing output image as transfer source
		VulkanTools::setImageLayout(
		    cmdBuffer,
		    storage_image.image,
		    VK_IMAGE_LAYOUT_GENERAL,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

		VkImageCopy copy_region{};
		copy_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy_region.srcOffset = {0, 0, 0};
		copy_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy_region.dstOffset = {0, 0, 0};
		copy_region.extent = {width, height, 1};
		vkCmdCopyImage(cmdBuffer,
		               storage_image.image,
		               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               dstImage,
		               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1,
		               &copy_region);

		// Transition destination image back to original layout
		VulkanTools::setImageLayout(
		    cmdBuffer,
		    dstImage,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    initialLayout,
		    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

		// Transition ray tracing output image back to general layout
		VulkanTools::setImageLayout(
		    cmdBuffer,
		    storage_image.image,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    VK_IMAGE_LAYOUT_GENERAL,
		    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
	}

	// Created the Bottom and Top Level Acceleration Structures needed for ray tracing.
	void RayTracingRenderSystem::createAccelerationStructures(std::shared_ptr<Scene>& scene) {
		createBLAS(scene);
		createTLAS(scene);

		createDescriptorSetLayout();
		createDescriptorSet(scene);

		createPipelineLayout(globalDescriptorSetLayouts);
		createPipelines();

		createShaderBindingTable();
	};

	// Creates the Bottom Level Acceleration Structures.
	// Adapted From: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
	void RayTracingRenderSystem::createBLAS(std::shared_ptr<Scene>& scene) {
		// BLAS - Storing each primitive in a geometry
		std::vector<BLASInput> BLASinputs;

		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;

		auto group = scene->getRenderComponents();
		for (const auto& entity : group) {
			auto& mesh = group.get<MeshComponent>(entity);
			BLASinputs.push_back(objectToGeometry(scene, vertexOffset, indexOffset, mesh));

			vertexOffset += static_cast<uint32_t>(mesh.vertices.size()) * sizeof(MeshComponent::Vertex);
			indexOffset += static_cast<uint32_t>(mesh.indices.size()) * sizeof(uint32_t);
		}

		uint32_t nbBlas = static_cast<uint32_t>(BLASinputs.size());
		VkDeviceSize asTotalSize{0};    // Memory size of all allocated BLAS
		uint32_t nbCompactions{0};      // Nb of BLAS requesting compaction
		VkDeviceSize maxScratchSize{0}; // Largest scratch size

		// Preparing the information for the acceleration build commands.
		std::vector<BuildAccelerationStructure> buildAS(nbBlas);
		for (uint32_t i = 0; i < nbBlas; i++) {
			// Filling partially the VkAccelerationStructureBuildGeometryInfoKHR for querying the build sizes.
			// Other information will be filled in later
			buildAS[i].buildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			buildAS[i].buildGeomInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			buildAS[i].buildGeomInfo.flags = BLASinputs[i].ASFlags | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			buildAS[i].buildGeomInfo.geometryCount = static_cast<uint32_t>(BLASinputs[i].ASGeometry.size());
			buildAS[i].buildGeomInfo.pGeometries = BLASinputs[i].ASGeometry.data();

			// Build range information
			buildAS[i].buildRangeInfo = BLASinputs[i].ASBuildOffsetInfo.data();

			// Finding sizes to create acceleration structures and scratch
			std::vector<uint32_t> maxPrimCount(BLASinputs[i].ASBuildOffsetInfo.size());
			for (auto tt = 0; tt < BLASinputs[i].ASBuildOffsetInfo.size(); tt++) {
				maxPrimCount[tt] = BLASinputs[i].ASBuildOffsetInfo[tt].primitiveCount; // Number of primitives/triangles
			}
			deviceProcedures.vkGetAccelerationStructureBuildSizesKHR(device.device(),
			                                                         VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			                                                         &buildAS[i].buildGeomInfo,
			                                                         maxPrimCount.data(),
			                                                         &buildAS[i].buildSizeInfo);

			// Extra info
			asTotalSize += buildAS[i].buildSizeInfo.accelerationStructureSize;
			maxScratchSize = std::max(maxScratchSize, buildAS[i].buildSizeInfo.buildScratchSize);
			if (buildAS[i].buildGeomInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR) {
				nbCompactions++;
			}
		}

		// Allocate the scratch buffers holding the temporary data of the acceleration structure builder
		Buffer scratchBuffer{
		    device,
		    maxScratchSize,
		    1,
		    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		};
		VkDeviceAddress scratchAddress = device.getBufferDeviceAddress(scratchBuffer.getBuffer());

		// Allocate a query pool for storing the needed size for every BLAS compaction.
		// vkGetAccelerationStructureBuildSizesKHR returns the worst case size. Query Pool is used to find actual size in order to perform compaction.
		VkQueryPool queryPool{VK_NULL_HANDLE};
		if (nbCompactions > 0) {             // Is compaction requested?
			assert(nbCompactions == nbBlas); // Don't allow mix of on/off compaction
			VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
			qpci.queryCount = nbBlas;
			qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
			vkCreateQueryPool(device.device(), &qpci, nullptr, &queryPool);
		}

		// Batching creation/compaction of BLAS to allow staying in restricted amount of memory
		std::vector<uint32_t> indices; // Indices of the BLAS to create
		VkDeviceSize batchSize{0};
		VkDeviceSize batchLimit{256'000'000}; // 256 MB
		for (uint32_t idx = 0; idx < nbBlas; idx++) {
			indices.push_back(idx);
			batchSize += buildAS[idx].buildSizeInfo.accelerationStructureSize;
			// Over the limit or last BLAS element
			if (batchSize >= batchLimit || idx == nbBlas - 1) {
				VkCommandBuffer cmdBuffer = device.beginSingleTimeCommandBuffers();
				cmdCreateBLAS(cmdBuffer, indices, buildAS, scratchAddress, queryPool);
				device.endSingleTimeCommandBuffers(cmdBuffer);

				if (queryPool) {
					VkCommandBuffer cmdBuffer = device.beginSingleTimeCommandBuffers();
					cmdCompactBLAS(cmdBuffer, indices, buildAS, queryPool);
					device.endSingleTimeCommandBuffers(cmdBuffer); // Submit command buffer and call vkQueueWaitIdle

					// Destroy all the non-compacted acceleration structures
					for (auto& idxj : indices) {
						buildAS[idxj].oldAccelerationStructure.buffer.reset();
						deviceProcedures.vkDestroyAccelerationStructureKHR(device.device(), buildAS[idxj].oldAccelerationStructure.handle, nullptr);
					}
				}

				// Reset
				batchSize = 0;
				indices.clear();
			}
		}

		// Keep all the created bottom level acceleration structures
		m_BLAS.resize(buildAS.size());
		for (int idx = 0; idx < buildAS.size(); idx++) {
			m_BLAS[idx] = std::move(buildAS[idx].accelerationStructure);
		}

		// Cleanup resources. The scratch buffer will get destroyed (in its destructor) when it goes out of scope.
		vkDestroyQueryPool(device.device(), queryPool, nullptr);
	}

	// Adapted From: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
	BLASInput RayTracingRenderSystem::objectToGeometry(std::shared_ptr<Scene>& scene, uint32_t vertexOffset, uint32_t indexOffset, MeshComponent& model) {
		// BLAS builder requires raw device addresses.
		VkDeviceOrHostAddressConstKHR vertexAddress;
		VkDeviceOrHostAddressConstKHR indexAddress;

		vertexAddress.deviceAddress = device.getBufferDeviceAddress(scene->getSceneData().vertexBuffer->getBuffer());
		indexAddress.deviceAddress = device.getBufferDeviceAddress(scene->getSceneData().indexBuffer->getBuffer());

		// Describe buffer as array of VertexObj.
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT; // vec3 vertex position data.
		triangles.vertexData.deviceAddress = vertexAddress.deviceAddress;
		triangles.vertexStride = sizeof(MeshComponent::Vertex);
		triangles.maxVertex = static_cast<uint32_t>(model.vertices.size());
		// Describe index data (32-bit unsigned int)
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress = indexAddress.deviceAddress;
		// Indicate identity transform by setting transformData to null device pointer.
		// triangles.transformData = {};

		// Identify the above data as containing opaque triangles.
		VkAccelerationStructureGeometryKHR asGeom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		asGeom.geometry.triangles = triangles;

		// The entire array will be used to build the BLAS.
		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = vertexOffset / sizeof(MeshComponent::Vertex);
		offset.primitiveCount = static_cast<uint32_t>(model.indices.size()) / 3;
		offset.primitiveOffset = indexOffset;
		offset.transformOffset = 0;

		// Our blas is made from only one geometry, but could be made of many geometries
		BLASInput input;
		input.ASGeometry.emplace_back(asGeom);
		input.ASBuildOffsetInfo.emplace_back(offset);
		// input.ASFlags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

		return input;
	}

	//--------------------------------------------------------------------------------------------------
	// Creating the bottom level acceleration structure for all indices of `buildAs` vector.
	// The array of BuildAccelerationStructure was created in buildBlas and the vector of
	// indices limits the number of BLAS to create at once. This limits the amount of
	// memory needed when compacting the BLAS.
	// Adapted From: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
	void RayTracingRenderSystem::cmdCreateBLAS(VkCommandBuffer cmdBuffer, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAS, VkDeviceAddress scratchAddress, VkQueryPool queryPool) {
		if (queryPool) { // For querying the compaction size
			vkResetQueryPool(device.device(), queryPool, 0, static_cast<uint32_t>(indices.size()));
		}
		uint32_t queryCnt{0};

		for (const auto& idx : indices) {
			// Create a individual buffer for a BLAS.
			buildAS[idx].accelerationStructure.buffer = std::make_unique<Buffer>(
			    device,
			    buildAS[idx].buildSizeInfo.accelerationStructureSize,
			    1,
			    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Actual allocation of buffer and acceleration structure.
			VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationStructureCreateInfo.size = buildAS[idx].buildSizeInfo.accelerationStructureSize; // Will be used to allocate memory.
			accelerationStructureCreateInfo.buffer = buildAS[idx].accelerationStructure.buffer->getBuffer();
			deviceProcedures.vkCreateAccelerationStructureKHR(device.device(), &accelerationStructureCreateInfo, nullptr, &buildAS[idx].accelerationStructure.handle);

			// Fill in the rest of the BuildInfo from earlier.
			buildAS[idx].buildGeomInfo.dstAccelerationStructure = buildAS[idx].accelerationStructure.handle; // Setting where the build lands
			buildAS[idx].buildGeomInfo.scratchData.deviceAddress = scratchAddress;                           // All build are using the same scratch buffer

			// Building the bottom-level-acceleration-structure
			deviceProcedures.vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildAS[idx].buildGeomInfo, &buildAS[idx].buildRangeInfo);

			// Since the scratch buffer is reused across builds, we need a memory barrier to ensure one build is finished before starting the next one.
			VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
			barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			vkCmdPipelineBarrier(
			    cmdBuffer,
			    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			    0,
			    1,
			    &barrier,
			    0,
			    nullptr,
			    0,
			    nullptr);

			// Save the device address of the BLAS for use in the TLAS.
			VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
			addressInfo.accelerationStructure = buildAS[idx].accelerationStructure.handle;
			buildAS[idx].accelerationStructure.device_address = deviceProcedures.vkGetAccelerationStructureDeviceAddressKHR(device.device(), &addressInfo);

			if (queryPool) {
				// Add a query to find the 'real' amount of memory needed, use for compaction
				deviceProcedures.vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuffer, 1, &buildAS[idx].buildGeomInfo.dstAccelerationStructure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
			}
		}
	}

	//--------------------------------------------------------------------------------------------------
	// Create and replace a new acceleration structure and buffer based on the size retrieved by the Query.
	// Adapted From: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
	void RayTracingRenderSystem::cmdCompactBLAS(VkCommandBuffer cmdBuffer, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAS, VkQueryPool queryPool) {
		uint32_t queryCtn{0};
		std::vector<AccelerationStructure> cleanupAS; // previous AS to destroy

		// Get the compacted size result back
		std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
		vkGetQueryPoolResults(device.device(), queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize), compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

		for (auto idx : indices) {
			buildAS[idx].oldAccelerationStructure = std::move(buildAS[idx].accelerationStructure); // previous AS to destroy
			buildAS[idx].buildSizeInfo.accelerationStructureSize = compactSizes[queryCtn++];       // new reduced size

			// Create a individual buffer for a BLAS.
			buildAS[idx].accelerationStructure.buffer = std::make_unique<Buffer>(
			    device,
			    buildAS[idx].buildSizeInfo.accelerationStructureSize,
			    1,
			    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Creating a compact version of the AS
			// Actual allocation of buffer and acceleration structure.
			VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationStructureCreateInfo.size = buildAS[idx].buildSizeInfo.accelerationStructureSize; // Will be used to allocate memory.
			accelerationStructureCreateInfo.buffer = buildAS[idx].accelerationStructure.buffer->getBuffer();
			deviceProcedures.vkCreateAccelerationStructureKHR(device.device(), &accelerationStructureCreateInfo, nullptr, &buildAS[idx].accelerationStructure.handle);

			// Copy the original BLAS to a compact version
			VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
			copyInfo.src = buildAS[idx].buildGeomInfo.dstAccelerationStructure;
			copyInfo.dst = buildAS[idx].accelerationStructure.handle;
			copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
			deviceProcedures.vkCmdCopyAccelerationStructureKHR(cmdBuffer, &copyInfo);

			// Save the device address of the BLAS for use in the TLAS.
			VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
			addressInfo.accelerationStructure = buildAS[idx].accelerationStructure.handle;
			buildAS[idx].accelerationStructure.device_address = deviceProcedures.vkGetAccelerationStructureDeviceAddressKHR(device.device(), &addressInfo);
		}
	}

	// Creates the Top Level Acceleration Structure.
	// Adapted From: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
	void RayTracingRenderSystem::createTLAS(std::shared_ptr<Scene>& scene) {
		int index = 0;
		auto group = scene->getRenderComponents();
		for (const auto& entity : group) {
			auto& transform = group.get<TransformComponent>(entity);

			VkAccelerationStructureInstanceKHR rayInst{};
			rayInst.transform = VulkanTools::glmToTransformMatrixKHR(transform.transform()); // Position of the instance
			rayInst.instanceCustomIndex = index;                                             // gl_InstanceCustomIndexEXT. Used for instancing.
			rayInst.accelerationStructureReference = m_BLAS[index++].device_address;
			rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			rayInst.mask = 0xFF;                                //  Only be hit if rayMask & instance.mask != 0
			rayInst.instanceShaderBindingTableRecordOffset = 0; // We will use the same hit group for all objects
			m_TLASInstances.emplace_back(rayInst);
		}
		buildTLAS(m_TLASInstances, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, false);
	}

	// Updates the Top Level Acceleration Structure.
	void RayTracingRenderSystem::updateTLAS(std::shared_ptr<Scene>& scene) {
		bool updateRequired = false;
		int index = 0;
		auto group = scene->getRenderComponents();
		for (const auto& entity : group) {
			auto& transform = group.get<TransformComponent>(entity);

			if (!transform.isTransformUpdated) {
				index++;
				continue;
			}
			updateRequired = true;

			m_TLASInstances[index].transform = VulkanTools::glmToTransformMatrixKHR(transform.transform()); // Position of the instance
			transform.isTransformUpdated = false;
		}

		if (updateRequired) {
			buildTLAS(m_TLASInstances, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, true);
		}
	}

	void RayTracingRenderSystem::buildTLAS(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags, bool update) {
		// Cannot call buildTlas twice except to update.
		assert(m_TLAS.handle == VK_NULL_HANDLE || update);
		uint32_t countInstance = static_cast<uint32_t>(instances.size());

		// Command buffer to create the TLAS
		VkCommandBuffer cmdBuffer = device.beginSingleTimeCommandBuffers();

		// Create a buffer holding the actual instance data (matrices++) for use by the AS builder
		// Buffer of instances containing the matrices and BLAS ids
		if (update) {
			instancesBuffer.reset();
		}
		instancesBuffer = std::make_unique<Buffer>(device,
		                                           sizeof(VkAccelerationStructureInstanceKHR),
		                                           instances.size(),
		                                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		instancesBuffer->map();
		instancesBuffer->writeToBuffer((void*)instances.data());
		instancesBuffer->unmap();
		VkDeviceAddress instanceBufferAddress = device.getBufferDeviceAddress(instancesBuffer->getBuffer());

		// Make sure the copy of the instances buffer are copied before triggering the acceleration structure build
		VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		vkCmdPipelineBarrier(cmdBuffer,
		                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		                     0,
		                     1,
		                     &barrier,
		                     0,
		                     nullptr,
		                     0,
		                     nullptr);

		// Wraps a device pointer to the above uploaded instances.
		VkAccelerationStructureGeometryInstancesDataKHR instancesVk{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
		instancesVk.data.deviceAddress = instanceBufferAddress;

		// Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
		VkAccelerationStructureGeometryKHR topASGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
		topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		topASGeometry.geometry.instances = instancesVk;

		// Find sizes
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
		buildInfo.flags = flags;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &topASGeometry;
		buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		deviceProcedures.vkGetAccelerationStructureBuildSizesKHR(device.device(),
		                                                         VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		                                                         &buildInfo,
		                                                         &countInstance,
		                                                         &sizeInfo);

		if (!update) {
			// Create a individual buffer for a TLAS.
			m_TLAS.buffer = std::make_unique<Buffer>(
			    device,
			    sizeInfo.accelerationStructureSize,
			    1,
			    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Actual allocation of buffer and acceleration structure.
			VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			accelerationStructureCreateInfo.size = sizeInfo.accelerationStructureSize; // Will be used to allocate memory.
			accelerationStructureCreateInfo.buffer = m_TLAS.buffer->getBuffer();
			deviceProcedures.vkCreateAccelerationStructureKHR(device.device(), &accelerationStructureCreateInfo, nullptr, &m_TLAS.handle);
		}

		// Allocate the scratch buffers holding the temporary data of the acceleration structure builder
		Buffer scratchBuffer{
		    device,
		    sizeInfo.buildScratchSize,
		    1,
		    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		};
		VkDeviceAddress scratchAddress = device.getBufferDeviceAddress(scratchBuffer.getBuffer());

		// Update build information
		buildInfo.srcAccelerationStructure = update ? m_TLAS.handle : VK_NULL_HANDLE;
		buildInfo.dstAccelerationStructure = m_TLAS.handle;
		buildInfo.scratchData.deviceAddress = scratchAddress;

		// Build Offsets info: n instances
		VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo{countInstance, 0, 0, 0};
		const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

		// Build the TLAS
		deviceProcedures.vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &pBuildOffsetInfo);

		device.endSingleTimeCommandBuffers(cmdBuffer);
	}

	/*
	    Create the Shader Binding Tables that connects the ray tracing pipelines' programs and the  top-level acceleration structure.
	    From: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

	    SBT Layout used in this sample:

	        /-----------\
	        | raygen    |
	        |-----------|
	        | raygen    |
	        |-----------|
	        | miss      |
	        |-----------|
	        | hit       |
	        \-----------/
	*/
	void RayTracingRenderSystem::createShaderBindingTable() {
		uint32_t missCount{1};
		uint32_t hitCount{1};
		auto handleCount = 1 + missCount + hitCount;
		uint32_t handleSize = device.rtProperties.shaderGroupHandleSize;
		// The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
		uint32_t handleSizeAligned = VulkanTools::alignedSize(handleSize, device.rtProperties.shaderGroupHandleAlignment);

		rgenRegion.stride = VulkanTools::alignedSize(handleSizeAligned, device.rtProperties.shaderGroupBaseAlignment);
		rgenRegion.size = rgenRegion.stride; // The size member of pRayGenShaderBindingTable must be equal to its stride member
		missRegion.stride = handleSizeAligned;
		missRegion.size = VulkanTools::alignedSize(missCount * handleSizeAligned, device.rtProperties.shaderGroupBaseAlignment);
		hitRegion.stride = handleSizeAligned;
		hitRegion.size = VulkanTools::alignedSize(hitCount * handleSizeAligned, device.rtProperties.shaderGroupBaseAlignment);

		// Get the shader group handles defined in the ray tracing pipeline.
		uint32_t dataSize = handleCount * handleSize;
		std::vector<uint8_t> handles(dataSize);
		VK_CHECK(deviceProcedures.vkGetRayTracingShaderGroupHandlesKHR(device.device(), pipeline.getPipeline(), 0, handleCount, dataSize, handles.data()));

		// Allocate a buffer for storing the SBT.
		VkDeviceSize sbtSize = rgenRegion.size + missRegion.size + hitRegion.size + callRegion.size;
		rtSBTBuffer = std::make_unique<Buffer>(device,
		                                       sbtSize,
		                                       1,
		                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Find the SBT addresses of each group
		VkDeviceAddress sbtAddress = device.getBufferDeviceAddress(rtSBTBuffer->getBuffer());
		rgenRegion.deviceAddress = sbtAddress;
		missRegion.deviceAddress = sbtAddress + rgenRegion.size;
		hitRegion.deviceAddress = sbtAddress + rgenRegion.size + missRegion.size;

		// Helper to retrieve the handle data
		auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

		// Map the SBT buffer and write in the handles.
		rtSBTBuffer->map();
		auto* pSBTBuffer = reinterpret_cast<uint8_t*>(rtSBTBuffer->getMappedMemory());
		uint8_t* pData{nullptr};
		uint32_t handleIdx{0};

		// Raygen
		pData = pSBTBuffer;
		memcpy(pData, getHandle(handleIdx++), handleSize);

		// Miss
		pData = pSBTBuffer + rgenRegion.size;
		for (uint32_t c = 0; c < missCount; c++) {
			memcpy(pData, getHandle(handleIdx++), handleSize);
			pData += missRegion.stride;
		}

		// Hit
		pData = pSBTBuffer + rgenRegion.size + missRegion.size;
		for (uint32_t c = 0; c < hitCount; c++) {
			memcpy(pData, getHandle(handleIdx++), handleSize);
			pData += hitRegion.stride;
		}

		rtSBTBuffer->unmap();
	}

	void RayTracingRenderSystem::assignTextures(Scene& scene) {
		assert(scene.getSceneData().textureCount > 0 && "There must be at least one texture loaded.");

		std::vector<VkDescriptorImageInfo> descriptorImageInfos(scene.getSceneData().textureCount);

		int index = 0;
		auto group = scene.getRenderComponents();
		for (const auto& entity : group) {
			auto& mesh = group.get<MeshComponent>(entity);

			if (!mesh.texture.isTextureLoaded) {
				continue;
			}

			descriptorImageInfos[index].imageLayout = mesh.texture.imageLayout;
			descriptorImageInfos[index].imageView = mesh.texture.view;
			descriptorImageInfos[index].sampler = mesh.texture.sampler;
			++index;
		}

		for (int i = 0; i < textureDescriptorSets.size(); ++i) {
			DescriptorWriter(*textureDescriptorSetLayout, device.getDescriptorPool())
			    .writeImage(0, descriptorImageInfos.data(), scene.getSceneData().textureCount)
			    .build(textureDescriptorSets[i]);
		}
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void RayTracingRenderSystem::createDescriptorSetLayout() {
		rtDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		                            .addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR) // Binding 0: Ray Generation shader to access the acceleration structures.
		                            .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR)              // Binding 1: Ray Gen shader storage image for offscreen rendering.
		                            .addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)        // Binding 1: Ray Hit shader storage buffer for getting vertex information.
		                            .addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)        // Binding 1: Ray Hit shader storage buffer for getting index information.
		                            .addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)        // Binding 1: Ray Hit shader storage buffer for getting offset information.
		                            .addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)        // Binding 1: Ray Hit shader storage buffer for getting texture id information.
		                            .build();

		textureDescriptorSetLayout = DescriptorSetLayout::Builder(device)
		                                 .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, 32) // Binding 0: Fragment shader combined image sampler for textures.
		                                 .build();
	}

	// Create Descriptor Sets.
	void RayTracingRenderSystem::createDescriptorSet(std::shared_ptr<Scene>& scene) {
		VkWriteDescriptorSetAccelerationStructureKHR ASInfo{};
		ASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		ASInfo.accelerationStructureCount = 1;
		ASInfo.pAccelerationStructures = &m_TLAS.handle;
		ASInfo.pNext = VK_NULL_HANDLE;

		VkDescriptorImageInfo image_descriptor{};
		image_descriptor.imageView = storage_image.view;
		image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto vertexBufferInfo = scene->getSceneData().vertexBuffer->descriptorInfo();
		auto indexBufferInfo = scene->getSceneData().indexBuffer->descriptorInfo();
		auto offsetBufferInfo = scene->getSceneData().offsetBuffer->descriptorInfo();
		auto textureIDBufferInfo = scene->getSceneData().textureIDBuffer->descriptorInfo();

		DescriptorWriter(*rtDescriptorSetLayout, device.getDescriptorPool())
		    .writeAccelerationStructure(0, &ASInfo)
		    .writeImage(1, &image_descriptor)
		    .writeBuffer(2, &vertexBufferInfo)
		    .writeBuffer(3, &indexBufferInfo)
		    .writeBuffer(4, &offsetBufferInfo)
		    .writeBuffer(5, &textureIDBufferInfo)
		    .build(rtDescriptorSet);
	}

	// Create a pipeline layout.
	void RayTracingRenderSystem::createPipelineLayout(std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayouts) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                                                                                                       // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(PushConstantRay);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		    globalDescriptorSetLayouts[0]->getDescriptorSetLayout(),
		    rtDescriptorSetLayout->getDescriptorSetLayout(),
		    textureDescriptorSetLayout->getDescriptorSetLayout(),
		};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the ray tracing pipeline defined in aspen_pipeline.cpp
	void RayTracingRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");

		enum StageIndices {
			eRaygen,
			eMiss,
			// eMiss2,
			eClosestHit,
			eShaderGroupCount
		};

		// All stages
		std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};

		// Raygen
		stages[eRaygen] = pipeline.createShaderModule("assets/shaders/raytrace.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		// Miss
		stages[eMiss] = pipeline.createShaderModule("assets/shaders/raytrace.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
		// The second miss shader is invoked when a shadow ray misses the geometry. It simply indicates that no occlusion has been found
		// stages[eMiss2] = pipeline.createShaderModule("assets/shaders/raytraceShadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
		// Hit Group - Closest Hit
		stages[eClosestHit] = pipeline.createShaderModule("assets/shaders/raytrace.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

		// Shader groups
		VkRayTracingShaderGroupCreateInfoKHR group{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;

		// Raygen
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = eRaygen;
		rtShaderGroups.push_back(group);

		// Miss
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		group.generalShader = eMiss;
		rtShaderGroups.push_back(group);

		// Shadow Miss
		// group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		// group.generalShader = eMiss2;
		// rtShaderGroups.push_back(group);

		// closest hit shader
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = eClosestHit;
		rtShaderGroups.push_back(group);

		// // Each shader constant of a shader stage corresponds to one map entry
		// std::array<VkSpecializationMapEntry, 1> specializationMapEntries{};
		// // Shader bindings based on specialization constants are marked by the new "constant_id" layout qualifier:
		// //	layout (constant_id = 0) const int numLights = 10;

		// // Map entry for the lighting model to be used by the fragment shader
		// specializationMapEntries[0].constantID = 0;
		// specializationMapEntries[0].size = sizeof(specializationData.numLights);
		// specializationMapEntries[0].offset = 0;

		// // Prepare specialization info block for the shader stage
		// VkSpecializationInfo specializationInfo{};
		// specializationInfo.dataSize = sizeof(specializationData);
		// specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
		// specializationInfo.pMapEntries = specializationMapEntries.data();
		// specializationInfo.pData = &specializationData;

		// // Set the number of max lights.
		// specializationData.numLights = MAX_LIGHTS;

		RayTracingPipelineConfigInfo pipelineConfig{};
		Pipeline::defaultRayTracingPipelineConfigInfo(pipelineConfig);
		pipelineConfig.shaderGroups = rtShaderGroups;
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();

		pipeline.createRayTracingPipeline(pipelineConfig, pipeline.getPipeline());
	}

	RenderInfo RayTracingRenderSystem::prepareRenderInfo() {
		RenderInfo renderInfo{};
		renderInfo.renderPass = resources->renderPass;
		renderInfo.framebuffer = resources->framebuffer;

		std::vector<VkClearValue> clearValues{2};
		clearValues[0].color = {0.5f, 0.0f, 0.0f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};
		renderInfo.clearValues = clearValues;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(renderer.getSwapChainExtent().width);
		viewport.height = static_cast<float>(renderer.getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		renderInfo.viewport = viewport;

		renderInfo.scissorDimensions = VkRect2D{{0, 0}, renderer.getSwapChainExtent()};

		return renderInfo;
	}

	void RayTracingRenderSystem::render(FrameInfo& frameInfo) { // Flush changes to update on the GPU side.
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());

		std::vector<VkDescriptorSet> descriptorSetsCombined{frameInfo.descriptorSet[0], rtDescriptorSet, textureDescriptorSets[frameInfo.frameIndex]};
		vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getPipelineLayout(), 0, 3, descriptorSetsCombined.data(), 0, nullptr);

		PushConstantRay push{};

		auto pointLightGroup = frameInfo.scene->getPointLights();
		auto [pointLightProps, pointLightTransform] = pointLightGroup.get<PointLightComponent, TransformComponent>(pointLightGroup[0]);

		push.clearColor = glm::vec4{0.2f, 0.5f, 0.0f, 1.0f};
		push.lightPosition = pointLightTransform.translation;
		push.lightIntensity = pointLightProps.lightIntensity;

		vkCmdPushConstants(frameInfo.commandBuffer, pipeline.getPipelineLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstantRay), &push);

		// Trace Rays
		deviceProcedures.vkCmdTraceRaysKHR(frameInfo.commandBuffer, &rgenRegion, &missRegion, &hitRegion, &callRegion, renderer.getSwapChainExtent().width, renderer.getSwapChainExtent().height, 1);

		// Copy ray traced output to render pass's attachment.
		copyToImage(frameInfo.commandBuffer, resources->attachments[0].image, resources->attachments[0].description.finalLayout, resources->width, resources->height);
	}

	void RayTracingRenderSystem::onResize() {
		resources->clearFramebuffer();
		updateResources();
	}
} // namespace Aspen