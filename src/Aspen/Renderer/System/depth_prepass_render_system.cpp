#include "Aspen/Renderer/System/depth_prepass_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 MVPMatrix{1.0f};
	};

	DepthPrePassRenderSystem::DepthPrePassRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout)
	    : device(device), renderer(renderer) {
		createPipelineLayout(globalDescriptorSetLayout);
		createPipelines();
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void DepthPrePassRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          //   .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)           // Binding 0: Vertex shader uniform buffer
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 1: Fragment shader image sampler
		                          .build();
	}

	// Create Descriptor Set.
	void DepthPrePassRenderSystem::createDescriptorSet() {
		// auto bufferInfo = uboBuffer->descriptorInfo();
		auto depthPrePass = renderer.getDepthPrePass();
		DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
		    // .writeBuffer(0, &bufferInfo)
		    .writeImage(0, &depthPrePass.descriptor)
		    .build(descriptorSet);
	}

	// Create a pipeline layout.
	void DepthPrePassRenderSystem::createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                              // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalDescriptorSetLayout->getDescriptorSetLayout()};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void DepthPrePassRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderer.getDepthPrePass().renderPass;
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		pipelineConfig.colorBlendInfo.attachmentCount = 0;

		pipeline.createShaderModule("assets/shaders/depth_prepass_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

		pipeline.createGraphicsPipeline(pipelineConfig, pipeline.getPipeline());
	}

	void DepthPrePassRenderSystem::onResize() {
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

	void DepthPrePassRenderSystem::render(FrameInfo& frameInfo) {
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());

		auto group = frameInfo.scene->getRenderComponents();
		for (auto& entity : group) {
			auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

			SimplePushConstantData push{};
			// Projection, View, Model Transformation matrix.
			push.MVPMatrix = frameInfo.camera.getProjection() * frameInfo.camera.getView() * transform.transform();

			vkCmdPushConstants(frameInfo.commandBuffer, pipeline.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SimplePushConstantData), &push);
			Aspen::Model::bind(frameInfo.commandBuffer, mesh.vertexBuffer, mesh.indexBuffer);
			Aspen::Model::draw(frameInfo.commandBuffer, static_cast<uint32_t>(mesh.indices.size()));
		}
	}
} // namespace Aspen