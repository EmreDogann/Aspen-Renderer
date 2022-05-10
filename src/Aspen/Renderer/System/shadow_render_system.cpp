#include "Aspen/Renderer/System/shadow_render_system.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace Aspen {
	struct SimplePushConstantData {
		glm::vec4 lightPos{0.0f};
	};

	ShadowRenderSystem::ShadowRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout)
	    : device(device), renderer(renderer), uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT), uboDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT), resources(std::make_unique<Framebuffer>(device)) {
		for (int i = 0; i < uboBuffers.size(); ++i) {
			// Create a UBO buffer. This will just be one instance per frame.
			uboBuffers[i] = std::make_unique<Buffer>(
			    device,
			    sizeof(ShadowUbo),
			    1,
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // We could make it device local but the performance gains could be cancelled out from writing to the UBO every frame.
			    device.properties.limits.minUniformBufferOffsetAlignment);

			// Map the buffer's memory so we can begin writing to it.
			uboBuffers[i]->map();
		}
		createResources();

		createDescriptorSetLayout();
		createDescriptorSet();

		createPipelineLayout(globalDescriptorSetLayout[1]); // We only care about the dynamic UBO for this render system.
		createPipelines();
	}

	void ShadowRenderSystem::createResources() {
		AttachmentCreateInfo attachmentCreateInfo{};
		attachmentCreateInfo.format = VK_FORMAT_D32_SFLOAT;
		attachmentCreateInfo.width = SHADOW_WIDTH;
		attachmentCreateInfo.height = SHADOW_HEIGHT;
		attachmentCreateInfo.imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
		attachmentCreateInfo.layerCount = 6;
		attachmentCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		attachmentCreateInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentCreateInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentCreateInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentCreateInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		resources->createAttachment(attachmentCreateInfo);
		resources->createSampler(SHAODW_FILTER, SHAODW_FILTER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
		resources->createMultiViewRenderPass(attachmentCreateInfo.layerCount);
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void ShadowRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT) // Binding 0: Vertex shader uniform buffer
		                          .build();
	}

	// Create Descriptor Set.
	void ShadowRenderSystem::createDescriptorSet() {
		// Create descriptor sets for UBO.
		for (int i = 0; i < uboDescriptorSets.size(); ++i) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
			    .writeBuffer(0, &bufferInfo)
			    .build(uboDescriptorSets[i]);
		}
	}

	// Create a pipeline layout.
	void ShadowRenderSystem::createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                              // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{descriptorSetLayout->getDescriptorSetLayout(), globalDescriptorSetLayout->getDescriptorSetLayout()};
		// shadowMappingPipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
		omniShadowMappingPipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void ShadowRenderSystem::createPipelines() {
		// assert(shadowMappingPipeline.getPipelineLayout() != nullptr && "Cannot create depth pipeline before pipeline layout!");
		assert(omniShadowMappingPipeline.getPipelineLayout() != nullptr && "Cannot create stencil pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = resources->renderPass;
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		// pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		// pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

		// pipelineConfig.pipelineLayout = shadowMappingPipeline.getPipelineLayout();
		// shadowMappingPipeline.createShaderModule("assets/shaders/depth_prepass_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		// shadowMappingPipeline.createGraphicsPipeline(pipelineConfig, shadowMappingPipeline.getPipeline());

		// Omni Shadow Mapping Configuration
		pipelineConfig.pipelineLayout = omniShadowMappingPipeline.getPipelineLayout();
		omniShadowMappingPipeline.createShaderModule("assets/shaders/shadow_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		omniShadowMappingPipeline.createShaderModule("assets/shaders/shadow_shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		omniShadowMappingPipeline.createGraphicsPipeline(pipelineConfig, omniShadowMappingPipeline.getPipeline());
	}

	RenderInfo ShadowRenderSystem::prepareRenderInfo() {
		RenderInfo renderInfo{};
		renderInfo.renderPass = resources->renderPass;
		renderInfo.framebuffer = resources->framebuffer;

		std::vector<VkClearValue> clearValues{1};
		// clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
		clearValues[0].depthStencil = {1.0f, 0};
		renderInfo.clearValues = clearValues;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = SHADOW_WIDTH;
		viewport.height = SHADOW_HEIGHT;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		renderInfo.viewport = viewport;

		renderInfo.scissorDimensions = VkRect2D{{0, 0}, {SHADOW_WIDTH, SHADOW_HEIGHT}};

		return renderInfo;
	}

	void ShadowRenderSystem::updateUBOs(FrameInfo& frameInfo) {
		ShadowUbo shadowUbo{};

		// Invert X and half Z.
		const glm::mat4 clip(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);

		shadowUbo.projectionMatrix = clip * glm::perspective(static_cast<float>(glm::pi<float>() / 2.0f), 1.0f, 0.01f, 25.0f);

		for (int i = 0; i < 6; ++i) {
			glm::mat4 viewMatrix = glm::mat4(1.0f);
			// glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3{0.0f, -1.0f, 2.5f});
			// glm::vec3 lightPos = glm::vec3{0.0f, -1.0f, 2.5f};
			auto pointLightGroup = frameInfo.scene->getPointLights();
			auto& pointLightTransform = pointLightGroup.get<TransformComponent>(pointLightGroup[0]);
			glm::vec3 lightPos = pointLightTransform.translation;

			switch (i) {
				case 0: // POSITIVE_X
					viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 1: // NEGATIVE_X
					viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 2: // POSITIVE_Y
					viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 3: // NEGATIVE_Y
					viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 4: // POSITIVE_Z
					viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 5: // NEGATIVE_Z
					viewMatrix = glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					break;
			}
			shadowUbo.viewMatries[i] = viewMatrix;
			// shadowUbo.viewMatries[i] = glm::inverse(viewMatrix);
		}

		uboBuffers[frameInfo.frameIndex]->writeToBuffer(&shadowUbo); // Write info to the UBO.
		uboBuffers[frameInfo.frameIndex]->flush();
	}

	void ShadowRenderSystem::render(FrameInfo& frameInfo) {
		// Bind the graphics pipieline.
		omniShadowMappingPipeline.bind(frameInfo.commandBuffer, omniShadowMappingPipeline.getPipeline());

		auto pointLightGroup = frameInfo.scene->getPointLights();
		for (const auto& pointLightEntity : pointLightGroup) {
			auto& pointLightTransform = pointLightGroup.get<TransformComponent>(pointLightEntity);
			int index = 0;
			auto renderGroup = frameInfo.scene->getRenderComponents();
			for (const auto& entity : renderGroup) {
				uint32_t dynamicOffset = index * frameInfo.dynamicOffset;
				std::vector<VkDescriptorSet> descriptorSetsCombined{uboDescriptorSets[frameInfo.frameIndex], frameInfo.descriptorSet[1]};
				vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, omniShadowMappingPipeline.getPipelineLayout(), 0, 2, descriptorSetsCombined.data(), 1, &dynamicOffset);

				auto [transform, mesh] = renderGroup.get<TransformComponent, MeshComponent>(entity);

				SimplePushConstantData push{};
				push.lightPos = glm::vec4(pointLightTransform.translation, 1.0f);
				vkCmdPushConstants(frameInfo.commandBuffer, omniShadowMappingPipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SimplePushConstantData), &push);

				Aspen::Model::bind(frameInfo.commandBuffer, mesh.vertexBuffer, mesh.indexBuffer);
				Aspen::Model::draw(frameInfo.commandBuffer, static_cast<uint32_t>(mesh.indices.size()));

				++index;
			}
		}
	}

	void ShadowRenderSystem::onResize() {
		// resources->clearFramebuffer();
		// createResources();
	}
} // namespace Aspen