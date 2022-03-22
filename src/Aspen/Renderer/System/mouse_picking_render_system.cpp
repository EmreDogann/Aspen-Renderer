#include "Aspen/Renderer/System/mouse_picking_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		alignas(16) uint64_t objectId;
	};

	MousePickingRenderSystem::MousePickingRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout)
	    : device(device), renderer(renderer) {
		// Create a UBO buffer. This will just be one instance per frame.
		storageBuffer = std::make_unique<Buffer>(
		    device,
		    sizeof(MousePickingStorageBuffer),
		    1,
		    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		    device.properties.limits.minUniformBufferOffsetAlignment);

		// Map the buffer's memory so we can begin writing to it.
		storageBuffer->map();

		renderer.createMousePickingPass(mousePickingResources);

		createDescriptorSetLayout();
		createDescriptorSet();

		createPipelineLayout(globalDescriptorSetLayout);
		createPipelines();
	}

	MousePickingRenderSystem::~MousePickingRenderSystem() {
		// Color Attachment
		vkDestroyImageView(device.device(), mousePickingResources.color.view, nullptr);
		vkDestroyImage(device.device(), mousePickingResources.color.image, nullptr);
		vkFreeMemory(device.device(), mousePickingResources.color.memory, nullptr);

		// Depth Attachment
		vkDestroyImageView(device.device(), mousePickingResources.depth.view, nullptr);
		vkDestroyImage(device.device(), mousePickingResources.depth.image, nullptr);
		vkFreeMemory(device.device(), mousePickingResources.depth.memory, nullptr);

		// Framebuffer
		vkDestroyFramebuffer(device.device(), mousePickingResources.frameBuffer, nullptr);

		// Renderpass
		vkDestroyRenderPass(device.device(), mousePickingResources.renderPass, nullptr);
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
		auto depthPrePass = renderer.getDepthPrePass();
		DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
		    .writeBuffer(0, &bufferInfo)
		    // .writeImage(1, &depthPrePass.descriptor)
		    .build(descriptorSet);
	}

	// Create a pipeline layout.
	void MousePickingRenderSystem::createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                              // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout->getDescriptorSetLayout(), descriptorSetLayout->getDescriptorSetLayout()};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void MousePickingRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = mousePickingResources.renderPass;
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

		pipeline.createShaderModule("assets/shaders/mouse_picking_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		pipeline.createShaderModule("assets/shaders/mouse_picking_shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		pipeline.createGraphicsPipeline(pipelineConfig, pipeline.getPipeline());
	}

	void MousePickingRenderSystem::onResize() {
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

	void MousePickingRenderSystem::render(FrameInfo& frameInfo) { // Flush changes to update on the GPU side.
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());
		vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipelineLayout(), 0, 2, std::array<VkDescriptorSet, 2>{frameInfo.descriptorSet, descriptorSet}.data(), 0, nullptr);

		auto group = frameInfo.scene->getRenderComponents();
		for (auto& entity : group) {
			auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
			// transform.rotation.y = glm::mod(transform.rotation.y + 0.0001f, glm::two_pi<float>());  // Slowly rotate game objects.
			// transform.rotation.x = glm::mod(transform.rotation.x + 0.00003f, glm::two_pi<float>()); // Slowly rotate game objects.

			SimplePushConstantData push{};
			// Projection, View, Model Transformation matrix.
			push.modelMatrix = transform.transform();
			push.objectId = static_cast<int64_t>(entity);

			vkCmdPushConstants(frameInfo.commandBuffer, pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SimplePushConstantData), &push);
			Aspen::Model::bind(frameInfo.commandBuffer, mesh.vertexBuffer, mesh.indexBuffer);
			Aspen::Model::draw(frameInfo.commandBuffer, static_cast<uint32_t>(mesh.indices.size()));
		}
	}

	// void MousePickingRenderSystem::renderUI(VkCommandBuffer commandBuffer) {
	// 	// Bind the graphics pipieline.
	// 	// vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &offscreenDescriptorSets[0], 0, nullptr);
	// 	pipeline->bind(commandBuffer, pipeline->getPresentPipeline());
	// }
} // namespace Aspen