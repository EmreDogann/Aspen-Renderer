#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	// Host data to take specialization constants from
	struct SpecializationData {
		// Sets the lighting model used in the fragment "uber" shader
		int numLights;
	};

	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout);
		~SimpleRenderSystem() = default;

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		SimpleRenderSystem(SimpleRenderSystem&&) = delete;            // Move Constructor
		SimpleRenderSystem& operator=(SimpleRenderSystem&&) = delete; // Move Assignment Operator

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

		SpecializationData specializationData{};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		std::vector<VkDescriptorSet> descriptorSets;

		std::vector<std::unique_ptr<Buffer>> uboBuffers;
	};
} // namespace Aspen