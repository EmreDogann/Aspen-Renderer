#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	class OutlineRenderSystem {
	public:
		OutlineRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout, std::shared_ptr<Framebuffer> resources);
		~OutlineRenderSystem() = default;

		OutlineRenderSystem(const OutlineRenderSystem&) = delete;
		OutlineRenderSystem& operator=(const OutlineRenderSystem&) = delete;

		OutlineRenderSystem(OutlineRenderSystem&&) = delete;            // Move Constructor
		OutlineRenderSystem& operator=(OutlineRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo, entt::entity selectedEntity);
		RenderInfo prepareRenderInfo();
		void onResize();

		VkDescriptorSet getDescriptorSet() {
			return descriptorSet;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout);

		Device& device;
		Renderer& renderer;
		std::weak_ptr<Framebuffer> resourcesSimpleRender;
		Pipeline pipeline{device};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		VkDescriptorSet descriptorSet;
	};
} // namespace Aspen