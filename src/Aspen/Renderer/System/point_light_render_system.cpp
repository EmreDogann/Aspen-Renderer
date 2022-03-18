#include "Aspen/Renderer/System/point_light_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::vec4 color{1.0f};
		alignas(16) glm::vec3 position{0.0f};
		alignas(4) float radius{0.0f};
	};

	PointLightRenderSystem::PointLightRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout)
	    : device(device), renderer(renderer), uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT), descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT) {

		createPipelineLayout(descriptorSetLayout);
		createPipelines();
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void PointLightRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 0: Vertex shader uniform buffer
		                          .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)                      // Binding 1: Fragment shader image sampler
		                          .build();
	}

	// Create Descriptor Sets.
	void PointLightRenderSystem::createDescriptorSet() {
		for (int i = 0; i < descriptorSets.size(); ++i) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
			    .writeBuffer(0, &bufferInfo)
			    .writeImage(1, &renderer.getOffscreenDescriptorInfo())
			    .build(descriptorSets[i]);
		}
	}

	// Create a pipeline layout.
	void PointLightRenderSystem::createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                                                             // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{descriptorSetLayout->getDescriptorSetLayout()};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void PointLightRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderer.getPresentRenderPass();
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();

		// We will not be passing in any vertex data into this vertex shader.
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.attributeDescriptions.clear();

		pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;
		pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		pipeline.createShaderModule(pipeline.getVertShaderModule(), "assets/shaders/point_light_shader.vert.spv");
		pipeline.createShaderModule(pipeline.getFragShaderModule(), "assets/shaders/point_light_shader.frag.spv");

		pipeline.createGraphicsPipeline(pipelineConfig, pipeline.getPipeline());
	}

	void PointLightRenderSystem::onResize() {
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

	void PointLightRenderSystem::render(FrameInfo& frameInfo) {
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());
		vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipelineLayout(), 0, 1, &frameInfo.descriptorSet, 0, nullptr);

		auto group = frameInfo.scene->getPointLights();
		for (auto& entity : group) {
			auto [transform, pointLight] = group.get<TransformComponent, PointLightComponent>(entity);

			SimplePushConstantData push{};
			push.position = transform.translation;
			push.color = glm::vec4(pointLight.color, pointLight.lightIntensity);
			push.radius = 0.04f;

			vkCmdPushConstants(frameInfo.commandBuffer, pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
	}
} // namespace Aspen