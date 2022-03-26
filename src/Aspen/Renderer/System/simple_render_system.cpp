#include "Aspen/Renderer/System/simple_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		alignas(16) int imageIndex;
	};

	SimpleRenderSystem::SimpleRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout, std::shared_ptr<Framebuffer> resources)
	    : device(device), renderer(renderer), resources(std::make_unique<Framebuffer>(device)), resourcesDepthPrePass(resources), descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT) {

		createResources();

		createDescriptorSetLayout();
		// createDescriptorSet();

		createPipelineLayout(globalDescriptorSetLayout);
		createPipelines();
	}

	void SimpleRenderSystem::createResources() {
		// Color Attachment
		{
			AttachmentCreateInfo attachmentCreateInfo{};
			attachmentCreateInfo.format = renderer.getSwapChainImageFormat();
			attachmentCreateInfo.width = renderer.getSwapChainExtent().width;
			attachmentCreateInfo.height = renderer.getSwapChainExtent().height;
			attachmentCreateInfo.imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
			attachmentCreateInfo.layerCount = 1;
			attachmentCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			attachmentCreateInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentCreateInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentCreateInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentCreateInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			resources->createAttachment(attachmentCreateInfo);
		}

		// Depth Attachment
		{
			// AttachmentCreateInfo attachmentCreateInfo{};
			// VulkanTools::getSupportedDepthFormat(device.physicalDevice(), &attachmentCreateInfo.format);
			// attachmentCreateInfo.width = renderer.getSwapChainExtent().width;
			// attachmentCreateInfo.height = renderer.getSwapChainExtent().height;
			// attachmentCreateInfo.imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
			// attachmentCreateInfo.layerCount = 1;
			// attachmentCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			// attachmentCreateInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			// attachmentCreateInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			// attachmentCreateInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			// attachmentCreateInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			// resources->createAttachment(attachmentCreateInfo);

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

	void SimpleRenderSystem::assignTextures(Scene& scene) {
		std::vector<VkDescriptorImageInfo> descriptorImageInfos(4);
		std::vector<VkDescriptorImageInfo> samplerDescriptorImageInfos(4);

		int index = 0;
		auto group = scene.getRenderComponents();
		for (const auto& entity : group) {
			auto& mesh = group.get<MeshComponent>(entity);

			descriptorImageInfos[index].imageLayout = mesh.texture.imageLayout;
			descriptorImageInfos[index].imageView = mesh.texture.view;
			descriptorImageInfos[index].sampler = mesh.texture.sampler;
			++index;
		}

		for (int i = 0; i < descriptorSets.size(); ++i) {
			DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
			    .writeImage(0, descriptorImageInfos.data(), 4)
			    .build(descriptorSets[i]);
		}
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void SimpleRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT, 32) // Binding 0: Fragment shader combined image sampler. Make it an array of 4 image samplers.
		                          .build();
	}

	// Create Descriptor Sets.
	void SimpleRenderSystem::createDescriptorSet() {
		for (int i = 0; i < descriptorSets.size(); ++i) {
			auto bufferInfo = uboBuffer->descriptorInfo();

			DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
			    .writeBuffer(0, &bufferInfo)
			    .build(descriptorSets[i]);
		}
	}

	// Create a pipeline layout.
	void SimpleRenderSystem::createPipelineLayout(std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                                // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout[0]->getDescriptorSetLayout(), globalDescriptorSetLayout[1]->getDescriptorSetLayout(), descriptorSetLayout->getDescriptorSetLayout()};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void SimpleRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");

		// Each shader constant of a shader stage corresponds to one map entry
		std::array<VkSpecializationMapEntry, 1> specializationMapEntries{};
		// Shader bindings based on specialization constants are marked by the new "constant_id" layout qualifier:
		//	layout (constant_id = 0) const int numLights = 10;

		// Map entry for the lighting model to be used by the fragment shader
		specializationMapEntries[0].constantID = 0;
		specializationMapEntries[0].size = sizeof(specializationData.numLights);
		specializationMapEntries[0].offset = 0;

		// Prepare specialization info block for the shader stage
		VkSpecializationInfo specializationInfo{};
		specializationInfo.dataSize = sizeof(specializationData);
		specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
		specializationInfo.pMapEntries = specializationMapEntries.data();
		specializationInfo.pData = &specializationData;

		// Set the number of max lights.
		specializationData.numLights = MAX_LIGHTS;

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = resources->renderPass;
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		// pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		pipeline.createShaderModule("assets/shaders/simple_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		pipeline.createShaderModule("assets/shaders/simple_shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

		pipeline.createGraphicsPipeline(pipelineConfig, pipeline.getPipeline());
	}

	RenderInfo SimpleRenderSystem::prepareRenderInfo() {
		RenderInfo renderInfo{};
		renderInfo.renderPass = resources->renderPass;
		renderInfo.framebuffer = resources->framebuffer;

		std::vector<VkClearValue> clearValues{2};
		clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
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

	void SimpleRenderSystem::render(FrameInfo& frameInfo) { // Flush changes to update on the GPU side.
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());

		int index = 0;

		auto group = frameInfo.scene->getRenderComponents();
		for (const auto& entity : group) {
			uint32_t dynamicOffset = index * frameInfo.dynamicOffset;
			std::vector<VkDescriptorSet> descriptorSetsCombined{frameInfo.descriptorSet[0], frameInfo.descriptorSet[1], descriptorSets[frameInfo.frameIndex]};
			vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipelineLayout(), 0, 3, descriptorSetsCombined.data(), 1, &dynamicOffset);

			auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
			// transform.rotation.y = glm::mod(transform.rotation.y + 0.0001f, glm::two_pi<float>());  // Slowly rotate game objects.
			// transform.rotation.x = glm::mod(transform.rotation.x + 0.00003f, glm::two_pi<float>()); // Slowly rotate game objects.

			SimplePushConstantData push{};
			push.imageIndex = index;

			vkCmdPushConstants(frameInfo.commandBuffer, pipeline.getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
			Aspen::Model::bind(frameInfo.commandBuffer, mesh.vertexBuffer, mesh.indexBuffer);
			Aspen::Model::draw(frameInfo.commandBuffer, static_cast<uint32_t>(mesh.indices.size()));

			++index;
		}
	}

	void SimpleRenderSystem::onResize() {
		resources->clearFramebuffer();
		createResources();
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

	// void SimpleRenderSystem::renderUI(VkCommandBuffer commandBuffer) {
	// 	// Bind the graphics pipieline.
	// 	// vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &offscreenDescriptorSets[0], 0, nullptr);
	// 	pipeline->bind(commandBuffer, pipeline->getPresentPipeline());
	// }
} // namespace Aspen