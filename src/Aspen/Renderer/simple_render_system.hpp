#pragma once
#include "Aspen/Core/model.hpp"
#include "Aspen/Renderer/pipeline.hpp"
#include "Aspen/Renderer/renderer.hpp"
#include "Aspen/Scene/scene.hpp"
#include "Aspen/Renderer/frame_info.hpp"

// Libs & defines
#include <glm/gtc/constants.hpp>

namespace Aspen {
	class SimpleRenderSystem {
	public:
		struct Ubo {
			alignas(16) glm::mat4 projectionView{1.0f};
			alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3(1.0f, -3.0f, -1.0f));
		};

		SimpleRenderSystem(Device& device, Renderer& renderer);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		SimpleRenderSystem(SimpleRenderSystem&&) = delete;            // Move Constructor
		SimpleRenderSystem& operator=(SimpleRenderSystem&&) = delete; // Move Assignment Operator

		void renderGameObjects(FrameInfo& frameInfo, std::shared_ptr<Scene>& scene);
		void renderUI(VkCommandBuffer commandBuffer);
		void onResize();

		VkDescriptorSet getCurrentDescriptorSet(int frameIndex) {
			return offscreenDescriptorSets[frameIndex];
		}

	private:
		void createPipelineLayout();
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();

		Device& device;
		Renderer& renderer;

		std::unique_ptr<Pipeline> pipeline;
		VkPipelineLayout pipelineLayout{};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		std::vector<VkDescriptorSet> offscreenDescriptorSets;

		std::vector<std::unique_ptr<Buffer>> uboBuffers;
		// Buffer uboBuffer{device, sizeof(Ubo), SwapChain::MAX_FRAMES_IN_FLIGHT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device.properties.limits.minUniformBufferOffsetAlignment};
	};
} // namespace Aspen