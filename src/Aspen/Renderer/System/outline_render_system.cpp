#include "Aspen/Renderer/System/outline_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 MVPMatrix{1.0f};
		alignas(4) float outlineWidth;
	};

	OutlineRenderSystem::OutlineRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout, std::shared_ptr<Framebuffer> resources)
	    : device(device), renderer(renderer), resourcesSimpleRender(resources) {

		// createResources();
		createPipelineLayout(globalDescriptorSetLayout);
		createPipelines();
	}

	// void OutlineRenderSystem::createResources() {
	// 	AttachmentCreateInfo attachmentCreateInfo{};
	// 	VulkanTools::getSupportedDepthFormat(device.physicalDevice(), &attachmentCreateInfo.format);
	// 	attachmentCreateInfo.width = renderer.getSwapChainExtent().width;
	// 	attachmentCreateInfo.height = renderer.getSwapChainExtent().height;
	// 	attachmentCreateInfo.imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
	// 	attachmentCreateInfo.layerCount = 1;
	// 	attachmentCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	// 	attachmentCreateInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// 	attachmentCreateInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// 	attachmentCreateInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// 	attachmentCreateInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// 	resources->createAttachment(attachmentCreateInfo);
	// 	resources->createRenderPass();
	// }

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void OutlineRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          //   .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)           // Binding 0: Vertex shader uniform buffer
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 1: Fragment shader image sampler
		                          .build();
	}

	// Create Descriptor Set.
	void OutlineRenderSystem::createDescriptorSet() {
		// auto bufferInfo = uboBuffer->descriptorInfo();
		// auto depthPrePass = renderer.getDepthPrePass();
		DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
		    // .writeBuffer(0, &bufferInfo)
		    // .writeImage(0, &depthPrePass.descriptor)
		    .build(descriptorSet);
	}

	// Create a pipeline layout.
	void OutlineRenderSystem::createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                              // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout->getDescriptorSetLayout()};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void OutlineRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");
		std::shared_ptr<Framebuffer> tempFramebuffer = resourcesSimpleRender.lock();

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = tempFramebuffer->renderPass;
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();
		// pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		// pipelineConfig.colorBlendInfo.attachmentCount = 0;

		pipelineConfig.depthStencilInfo.stencilTestEnable = VK_TRUE;
		pipelineConfig.depthStencilInfo.back.compareOp = VK_COMPARE_OP_NOT_EQUAL;
		pipelineConfig.depthStencilInfo.back.failOp = VK_STENCIL_OP_KEEP;
		pipelineConfig.depthStencilInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
		pipelineConfig.depthStencilInfo.back.passOp = VK_STENCIL_OP_REPLACE;
		pipelineConfig.depthStencilInfo.back.compareMask = 0xff;
		pipelineConfig.depthStencilInfo.back.writeMask = 0xff;
		pipelineConfig.depthStencilInfo.back.reference = 1;
		pipelineConfig.depthStencilInfo.front = pipelineConfig.depthStencilInfo.back;
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;

		pipeline.createShaderModule("assets/shaders/outline_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		pipeline.createShaderModule("assets/shaders/outline_shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		pipeline.createGraphicsPipeline(pipelineConfig, pipeline.getPipeline());
	}

	RenderInfo OutlineRenderSystem::prepareRenderInfo() {
		std::shared_ptr<Framebuffer> tempFramebuffer = resourcesSimpleRender.lock();

		RenderInfo renderInfo{};
		renderInfo.renderPass = tempFramebuffer->renderPass;
		renderInfo.framebuffer = tempFramebuffer->framebuffer;

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

		renderInfo.scissorDimensions = VkRect2D{{0, 0}, renderer.getSwapChainExtent()};

		return renderInfo;
	}

	void OutlineRenderSystem::render(FrameInfo& frameInfo, entt::entity selectedEntity) {
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());

		auto group = frameInfo.scene->getRenderComponents();
		auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(selectedEntity);

		SimplePushConstantData push{};
		// Projection, View, Model Transformation matrix.
		auto oldScale = transform.scale;
		transform.scale *= glm::vec3(1.02f);
		push.MVPMatrix = frameInfo.camera.getProjection() * frameInfo.camera.getView() * transform.transform();
		push.outlineWidth = 0.01f;
		transform.scale = oldScale;

		vkCmdPushConstants(frameInfo.commandBuffer, pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SimplePushConstantData), &push);
		Aspen::Model::bind(frameInfo.commandBuffer, mesh.vertexBuffer, mesh.indexBuffer);
		Aspen::Model::draw(frameInfo.commandBuffer, static_cast<uint32_t>(mesh.indices.size()));
	}

	void OutlineRenderSystem::onResize() {
		// resources->clearFramebuffer();
		// createResources();

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