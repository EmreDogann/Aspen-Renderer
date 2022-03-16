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
			glm::mat4 projectionViewMatrix{1.0f};
			glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.02f}; // w is intensity
			glm::vec4 lightColor{1.0f};                           // w is intensity
			alignas(16) glm::vec3 lightPosition{-1.0f, -2.0f, 2.0f};
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
		float moveLight = 0.0f;
		float moveDirection = 0.0001f;
	};
} // namespace Aspen