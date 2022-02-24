#pragma once
#include "Aspen/Core/model.hpp"
#include "Aspen/Renderer/camera.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/pipeline.hpp"
#include "Aspen/Scene/game_object.hpp"

// Libs & defines
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vulkan/vulkan_core.h>

namespace Aspen {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(AspenDevice &device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem &) = delete;
		SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

		SimpleRenderSystem(SimpleRenderSystem &&) = delete;            // Move Constructor
		SimpleRenderSystem &operator=(SimpleRenderSystem &&) = delete; // Move Assignment Operator

		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<AspenGameObject> &gameObjects, const AspenCamera &camera);

	private:
		void createPipelineLayout();
		void createDescriptorSetLayout();
		void createPipeline(VkRenderPass renderPass);

		AspenDevice &aspenDevice;

		std::unique_ptr<AspenPipeline> aspenPipeline;
		VkPipelineLayout pipelineLayout{};

		VkDescriptorSetLayout descriptorSetLayout{};
	};
} // namespace Aspen