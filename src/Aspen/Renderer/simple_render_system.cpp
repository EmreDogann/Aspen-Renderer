#include "Aspen/Renderer/simple_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
	};

	SimpleRenderSystem::SimpleRenderSystem(Device& device, Renderer& renderer)
	    : device(device), renderer(renderer), uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT), offscreenDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT) {
		for (int i = 0; i < uboBuffers.size(); ++i) {
			// Create a UBO buffer. This will just be one instance per frame.
			uboBuffers[i] = std::make_unique<Buffer>(
			    device,
			    sizeof(Ubo),
			    1,
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); // We could make it device local but the performance gains could be cancelled out from writing to the UBO every frame.

			// Map the buffer's memory so we can begin writing to it.
			uboBuffers[i]->map();
		}

		createPipelineLayout();
		pipeline = std::make_unique<Pipeline>(device, "assets/shaders/simple_shader.vert.spv", "assets/shaders/simple_shader.frag.spv");
		createPipelines();
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void SimpleRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)           // Binding 0: Vertex shader uniform buffer
		                          .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 1: Fragment shader image sampler
		                          .build();
	}

	// Create Descriptor Sets.
	void SimpleRenderSystem::createDescriptorSet() {
		for (int i = 0; i < offscreenDescriptorSets.size(); ++i) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
			    .writeBuffer(0, &bufferInfo)
			    .writeImage(1, &renderer.getOffscreenDescriptorInfo())
			    .build(offscreenDescriptorSets[i]);
		}
	}

	// Create a pipeline layout.
	void SimpleRenderSystem::createPipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                                                             // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		createDescriptorSetLayout();
		createDescriptorSet();

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{descriptorSetLayout->getDescriptorSetLayout()};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void SimpleRenderSystem::createPipelines() {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderer.getPresentRenderPass();
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline->createGraphicsPipeline(pipelineConfig, pipeline->getPresentPipeline());

		// pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		pipelineConfig.renderPass = renderer.getOffscreenRenderPass();
		pipeline->createGraphicsPipeline(pipelineConfig, pipeline->getOffscreenPipeline());
	}

	void SimpleRenderSystem::onResize() {
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

	void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo, std::shared_ptr<Scene>& scene) { // Flush changes to update on the GPU side.
		// Bind the graphics pipieline.
		pipeline->bind(frameInfo.commandBuffer, pipeline->getOffscreenPipeline());
		vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &offscreenDescriptorSets[frameInfo.frameIndex], 0, nullptr);

		// Update our UBO buffer.
		Ubo ubo{};
		ubo.projectionViewMatrix = frameInfo.camera.getProjection() * frameInfo.camera.getView(); // Calculate the projection view transformation matrix.

		if (moveLight <= -2.0f) {
			moveDirection *= -1;
		} else if (moveLight >= 1.0f) {
			moveDirection *= -1;
		}
		moveLight += moveDirection;

		ubo.lightDirection = glm::normalize(glm::vec3(ubo.lightDirection.x + moveLight, ubo.lightDirection.y, ubo.lightDirection.z));

		uboBuffers[frameInfo.frameIndex]->writeToBuffer(&ubo); // Write info to the UBO buffer.
		uboBuffers[frameInfo.frameIndex]->flush();

		auto group = scene->getRenderComponents();
		for (auto& entity : group) {
			auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
			// transform.rotation.y = glm::mod(transform.rotation.y + 0.0001f, glm::two_pi<float>());  // Slowly rotate game objects.
			// transform.rotation.x = glm::mod(transform.rotation.x + 0.00003f, glm::two_pi<float>()); // Slowly rotate game objects.

			SimplePushConstantData push{};
			// Projection, View, Model Transformation matrix.
			push.modelMatrix = transform.transform();
			push.normalMatrix = transform.computeNormalMatrix();

			vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
			Aspen::Model::bind(frameInfo.commandBuffer, mesh.vertexBuffer, mesh.indexBuffer);
			Aspen::Model::draw(frameInfo.commandBuffer, static_cast<uint32_t>(mesh.indices.size()));
		}
	}

	void SimpleRenderSystem::renderUI(VkCommandBuffer commandBuffer) {
		// Bind the graphics pipieline.
		// vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &offscreenDescriptorSets[0], 0, nullptr);
		pipeline->bind(commandBuffer, pipeline->getPresentPipeline());
	}
} // namespace Aspen