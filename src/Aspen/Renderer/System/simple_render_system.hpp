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
		SimpleRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout, std::shared_ptr<Framebuffer> resourcesDepthPrePass, std::shared_ptr<Framebuffer> resourcesShadow);
		~SimpleRenderSystem() = default;

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		SimpleRenderSystem(SimpleRenderSystem&&) = delete;            // Move Constructor
		SimpleRenderSystem& operator=(SimpleRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo);
		void createResources();
		void assignTextures(Scene& scene);
		RenderInfo prepareRenderInfo();
		void onResize();

		VkDescriptorSet getCurrentDescriptorSet(int frameIndex) {
			return textureDescriptorSets[frameIndex];
		}

		std::shared_ptr<Framebuffer> getResources() {
			return resources;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout(std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout);

		Device& device;
		Renderer& renderer;
		std::shared_ptr<Framebuffer> resources;
		std::weak_ptr<Framebuffer> resourcesDepthPrePass;
		std::weak_ptr<Framebuffer> resourcesShadow;
		Pipeline pipeline{device};

		SpecializationData specializationData{};

		// Shadows
		std::unique_ptr<DescriptorSetLayout> shadowDescriptorSetLayout{};
		std::vector<VkDescriptorSet> shadowDescriptorSets;

		// Textures
		std::unique_ptr<DescriptorSetLayout> textureDescriptorSetLayout{};
		std::vector<VkDescriptorSet> textureDescriptorSets;

		std::unique_ptr<Buffer> uboBuffer;
	};
} // namespace Aspen