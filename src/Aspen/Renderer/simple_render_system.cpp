#include "simple_render_system.hpp"
#include "vulkan/vulkan_core.h"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 transform{1.f};
		alignas(16) glm::vec3 color;
	};

	SimpleRenderSystem::SimpleRenderSystem(AspenDevice &device, VkRenderPass renderPass) : aspenDevice(device) {
		createPipelineLayout();
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyDescriptorSetLayout(aspenDevice.device(), descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(aspenDevice.device(), pipelineLayout, nullptr);
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO).
	void SimpleRenderSystem::createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(aspenDevice.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	// Create a pipeline layout.
	void SimpleRenderSystem::createPipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                                                             // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		createDescriptorSetLayout();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(aspenDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		AspenPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		aspenPipeline = std::make_unique<AspenPipeline>(aspenDevice, "assets/shaders/simple_shader.vert.spv", "assets/shaders/simple_shader.frag.spv", pipelineConfig);
	}

	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<AspenGameObject> &gameObjects, const AspenCamera &camera) {
		// Bind the graphics pipieline.
		aspenPipeline->bind(commandBuffer);

		// Calculate the projection view transformation matrix.
		auto projectionView = camera.getProjection() * camera.getView();

		for (auto &obj : gameObjects) {
			obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.0001f, glm::two_pi<float>());  // Slowly rotate game objects.
			obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.00003f, glm::two_pi<float>()); // Slowly rotate game objects.

			SimplePushConstantData push{};
			// Projection, View, Model Transformation matrix.
			push.transform = projectionView * obj.transform.mat4();
			push.color = obj.color;

			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}
} // namespace Aspen