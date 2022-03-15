#pragma once
#include "Aspen/Core/buffer.hpp"
#include "Aspen/Renderer/camera.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/pipeline.hpp"
#include "Aspen/Renderer/renderer.hpp"
#include "Aspen/Scene/scene.hpp"

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
		SimpleRenderSystem(Device& device, Buffer& bufferManager, Renderer& renderer);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		SimpleRenderSystem(SimpleRenderSystem&&) = delete;            // Move Constructor
		SimpleRenderSystem& operator=(SimpleRenderSystem&&) = delete; // Move Assignment Operator

		void renderGameObjects(VkCommandBuffer commandBuffer, std::shared_ptr<Scene>& scene, const Camera& camera);
		void renderUI(VkCommandBuffer commandBuffer);
		void onResize();

		VkDescriptorSet getCurrentDescriptorSet() {
			return offscreenDescriptorSets[0];
		}

	private:
		void createPipelineLayout();
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();

		Device& device;
		Buffer& bufferManager;
		Renderer& renderer;

		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout{};

		VkDescriptorSetLayout descriptorSetLayout{};
		std::vector<VkDescriptorSet> offscreenDescriptorSets;
	};
} // namespace Aspen