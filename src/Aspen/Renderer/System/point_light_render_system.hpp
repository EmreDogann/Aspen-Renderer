#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	class PointLightRenderSystem {
	public:
		PointLightRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout);
		~PointLightRenderSystem() = default;

		PointLightRenderSystem(const PointLightRenderSystem&) = delete;
		PointLightRenderSystem& operator=(const PointLightRenderSystem&) = delete;

		PointLightRenderSystem(PointLightRenderSystem&&) = delete;            // Move Constructor
		PointLightRenderSystem& operator=(PointLightRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo);
		void onResize();

		VkDescriptorSet getCurrentDescriptorSet(int frameIndex) {
			return descriptorSets[frameIndex];
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout);

		Device& device;
		Renderer& renderer;
		Pipeline pipeline{device};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		std::vector<VkDescriptorSet> descriptorSets;

		std::vector<std::unique_ptr<Buffer>> uboBuffers;
	};
} // namespace Aspen