#include "Aspen/Renderer/System/mouse_picking_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		alignas(16) uint64_t objectId;
	};

	MousePickingRenderSystem::MousePickingRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout, std::shared_ptr<Framebuffer> resources)
	    : device(device), renderer(renderer), resources(std::make_unique<Framebuffer>(device)), resourcesDepthPrePass(resources), globalDescriptorSetLayout(*globalDescriptorSetLayout) {
		// Create a UBO buffer. This will just be one instance per frame.
		{
			storageBuffer = std::make_unique<Buffer>(
			    device,
			    sizeof(MousePickingStorageBuffer),
			    1,
			    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			    device.properties.limits.minUniformBufferOffsetAlignment);

			// Map the buffer's memory so we can begin writing to it.
			storageBuffer->map();
		}

		loadAttachment();

		createDescriptorSetLayout();
		createDescriptorSet();

		createPipelineLayout();
		createPipelines();
	}

	void MousePickingRenderSystem::loadAttachment() {
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
		attachmentAddInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentAddInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		resources->addLoadAttachment(attachmentAddInfo);
		resources->createRenderPass();
	}

	// Create a Descriptor Set Layout for a Storage & Uniformbuffer.
	void MousePickingRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          //   .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 0: Vertex shader uniform buffer
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 0: Fragment shader storage buffer
		                                                                                                          //   .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 1: Fragment shader image sampler
		                          .build();
	}

	// Create Descriptor Sets.
	void MousePickingRenderSystem::createDescriptorSet() {
		auto bufferInfo = storageBuffer->descriptorInfo();
		// auto depthPrePass = renderer.getDepthPrePass();
		DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
		    .writeBuffer(0, &bufferInfo)
		    // .writeImage(1, &depthPrePass.descriptor)
		    .build(descriptorSet);
	}

	// Create a pipeline layout.
	void MousePickingRenderSystem::createPipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                              // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout.getDescriptorSetLayout(), descriptorSetLayout->getDescriptorSetLayout()};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void MousePickingRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = resources->renderPass;
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		// pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

		pipeline.createShaderModule("assets/shaders/mouse_picking_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		pipeline.createShaderModule("assets/shaders/mouse_picking_shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		pipeline.createGraphicsPipeline(pipelineConfig, pipeline.getPipeline());
	}

	RenderInfo MousePickingRenderSystem::prepareRenderInfo() {
		RenderInfo renderInfo{};
		renderInfo.renderPass = resources->renderPass;
		renderInfo.framebuffer = resources->framebuffer;

		std::vector<VkClearValue> clearValues{1};
		clearValues[0].depthStencil = {1.0f, 0};
		renderInfo.clearValues = clearValues;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(renderer.getSwapChainExtent().width);
		viewport.height = static_cast<float>(renderer.getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		renderInfo.viewport = viewport;

		renderInfo.scissorDimensions = VkRect2D{{static_cast<int32_t>(Input::GetMouseX() > 0 ? Input::GetMouseX() : 0), static_cast<int32_t>(Input::GetMouseY() > 0 ? Input::GetMouseY() : 0)}, {1, 1}};

		return renderInfo;
	}

	void MousePickingRenderSystem::render(FrameInfo& frameInfo) { // Flush changes to update on the GPU side.
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());
		vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipelineLayout(), 0, 2, std::array<VkDescriptorSet, 2>{frameInfo.descriptorSet, descriptorSet}.data(), 0, nullptr);

		auto group = frameInfo.scene->getRenderComponents();
		for (auto& entity : group) {
			auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

			SimplePushConstantData push{};
			// Projection, View, Model Transformation matrix.
			push.modelMatrix = transform.transform();
			push.objectId = static_cast<int64_t>(entity);

			vkCmdPushConstants(frameInfo.commandBuffer, pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SimplePushConstantData), &push);
			Aspen::Model::bind(frameInfo.commandBuffer, mesh.vertexBuffer, mesh.indexBuffer);
			Aspen::Model::draw(frameInfo.commandBuffer, static_cast<uint32_t>(mesh.indices.size()));
		}
	}

	void MousePickingRenderSystem::onResize() {
		resources->clearFramebuffer();
		loadAttachment();

		// for (int i = 0; i < offscreenDescriptorSets.size(); ++i) {
		// 	auto bufferInfo = uboBuffers[i]->descriptorInfo();
		// 	DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
		// 	    .writeBuffer(0, &bufferInfo)
		// 	    .writeImage(1, &renderer.getOffscreenDescriptorInfo())
		// 	    .overwrite(offscreenDescriptorSets[i]);
		// }

		// VkWriteDescriptorSet offScreenWriteDescriptorSets{};
		// offScreenWriteDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// offScreenWriteDescriptorSets.dstSet = offscreenDescriptorSets[0];
		// offScreenWriteDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// offScreenWriteDescriptorSets.dstBinding = 1;
		// offScreenWriteDescriptorSets.descriptorCount = 1;
		// offScreenWriteDescriptorSets.pImageInfo = &renderer.getOffscreenDescriptorInfo();

		// vkUpdateDescriptorSets(device.device(), 1, &offScreenWriteDescriptorSets, 0, nullptr);
	}
} // namespace Aspen