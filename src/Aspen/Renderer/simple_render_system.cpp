#include "Aspen/Renderer/simple_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 transform{1.0f};
		glm::mat4 normalMatrix{1.0f};
	};

	SimpleRenderSystem::SimpleRenderSystem(Device& device, Buffer& bufferManager, Renderer& renderer)
	    : device(device), bufferManager(bufferManager), renderer(renderer) {
		createPipelineLayout();
		pipeline = std::make_unique<Pipeline>(device, "assets/shaders/simple_shader.vert.spv", "assets/shaders/simple_shader.frag.spv");
		createPipelines();
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void SimpleRenderSystem::createDescriptorSetLayout() {
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		// Binding 0: Vertex shader uniform buffer
		setLayoutBindings.push_back(VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});

		// Binding 1: Fragment shader image sampler
		setLayoutBindings.push_back(VkDescriptorSetLayoutBinding{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		layoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(device.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	// Create Descriptor Sets.
	void SimpleRenderSystem::createDescriptorSet() {
		std::vector<VkDescriptorSetLayout> offscreenDescriptorSetLayout(renderer.getSwapChainImageCount(), descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = device.getDescriptorPool();
		allocInfo.pSetLayouts = offscreenDescriptorSetLayout.data();
		allocInfo.descriptorSetCount = renderer.getSwapChainImageCount();

		offscreenDescriptorSets.resize(renderer.getSwapChainImageCount());

		// Offscreen
		if (vkAllocateDescriptorSets(device.device(), &allocInfo, offscreenDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen descriptor set!");
		}

		VkWriteDescriptorSet offScreenWriteDescriptorSets{};
		offScreenWriteDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		offScreenWriteDescriptorSets.dstSet = offscreenDescriptorSets[0];
		offScreenWriteDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		offScreenWriteDescriptorSets.dstBinding = 1;
		offScreenWriteDescriptorSets.descriptorCount = 1;
		offScreenWriteDescriptorSets.pImageInfo = &renderer.getOffscreenDescriptorInfo();

		vkUpdateDescriptorSets(device.device(), 1, &offScreenWriteDescriptorSets, 0, nullptr);
	}

	// Create a pipeline layout.
	void SimpleRenderSystem::createPipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                                                             // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		createDescriptorSetLayout();
		createDescriptorSet();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
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
		VkWriteDescriptorSet offScreenWriteDescriptorSets{};
		offScreenWriteDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		offScreenWriteDescriptorSets.dstSet = offscreenDescriptorSets[0];
		offScreenWriteDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		offScreenWriteDescriptorSets.dstBinding = 1;
		offScreenWriteDescriptorSets.descriptorCount = 1;
		offScreenWriteDescriptorSets.pImageInfo = &renderer.getOffscreenDescriptorInfo();

		vkUpdateDescriptorSets(device.device(), 1, &offScreenWriteDescriptorSets, 0, nullptr);
	}

	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::shared_ptr<Scene>& scene, const Camera& camera) {
		// Bind the graphics pipieline.
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &offscreenDescriptorSets[0], 0, nullptr);
		pipeline->bind(commandBuffer, pipeline->getOffscreenPipeline());

		// Calculate the projection view transformation matrix.
		auto projectionView = camera.getProjection() * camera.getView();

		auto group = scene->getRenderComponents();
		for (auto& entity : group) {
			auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
			// transform.rotation.y = glm::mod(transform.rotation.y + 0.0001f, glm::two_pi<float>());  // Slowly rotate game objects.
			// transform.rotation.x = glm::mod(transform.rotation.x + 0.00003f, glm::two_pi<float>()); // Slowly rotate game objects.

			SimplePushConstantData push{};
			// Projection, View, Model Transformation matrix.
			auto modelMatrix = transform.transform();
			push.transform = projectionView * modelMatrix;
			push.normalMatrix = transform.computeNormalMatrix();

			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
			Aspen::Buffer::bind(commandBuffer, mesh.meshMemory);
			Aspen::Buffer::draw(commandBuffer, static_cast<uint32_t>(mesh.indices.size()));
		}
	}

	void SimpleRenderSystem::renderUI(VkCommandBuffer commandBuffer) {
		// Bind the graphics pipieline.
		// vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &offscreenDescriptorSets[0], 0, nullptr);
		pipeline->bind(commandBuffer, pipeline->getPresentPipeline());
	}
} // namespace Aspen