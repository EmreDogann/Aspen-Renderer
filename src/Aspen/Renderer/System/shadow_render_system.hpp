#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	class ShadowRenderSystem {
	public:
		static constexpr int SHADOW_WIDTH = 1024;
		static constexpr int SHADOW_HEIGHT = 1024;
		static constexpr VkFilter SHAODW_FILTER = VK_FILTER_LINEAR;

		struct ShadowUbo {
			glm::mat4 projectionMatrix{1.0f};
			glm::mat4 viewMatries[6];
		};

		ShadowRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout);
		~ShadowRenderSystem() = default;

		ShadowRenderSystem(const ShadowRenderSystem&) = delete;
		ShadowRenderSystem& operator=(const ShadowRenderSystem&) = delete;

		ShadowRenderSystem(ShadowRenderSystem&&) = delete;            // Move Constructor
		ShadowRenderSystem& operator=(ShadowRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo);
		void updateUBOs(FrameInfo& frameInfo);
		void createResources();
		RenderInfo prepareRenderInfo();
		void onResize();

		std::shared_ptr<Framebuffer> getResources() {
			return resources;
		}

		std::vector<VkDescriptorSet>& getDescriptorSets() {
			return uboDescriptorSets;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout);

		Device& device;
		Renderer& renderer;
		std::shared_ptr<Framebuffer> resources;
		Pipeline shadowMappingPipeline{device};
		Pipeline omniShadowMappingPipeline{device};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		std::vector<VkDescriptorSet> uboDescriptorSets;

		std::vector<std::unique_ptr<Buffer>> uboBuffers;
	};
} // namespace Aspen