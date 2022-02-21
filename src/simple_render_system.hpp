#pragma once
#include "aspen_camera.hpp"
#include "aspen_device.hpp"
#include "aspen_game_object.hpp"
#include "aspen_model.hpp"
#include "aspen_pipeline.hpp"

// Libs
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vulkan/vulkan_core.h>

// Lib Defines
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.

// std
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

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