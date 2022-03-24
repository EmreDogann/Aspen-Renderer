#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	class DepthPrePassRenderSystem {
	public:
		DepthPrePassRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout);
		~DepthPrePassRenderSystem() = default;

		DepthPrePassRenderSystem(const DepthPrePassRenderSystem&) = delete;
		DepthPrePassRenderSystem& operator=(const DepthPrePassRenderSystem&) = delete;

		DepthPrePassRenderSystem(DepthPrePassRenderSystem&&) = delete;            // Move Constructor
		DepthPrePassRenderSystem& operator=(DepthPrePassRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo, entt::entity selectedEntity);
		void createResources();
		RenderInfo prepareRenderInfo();
		void onResize();

		std::shared_ptr<Framebuffer> getResources() {
			return resources;
		}

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
		std::shared_ptr<Framebuffer> resources;
		Pipeline depthPipeline{device};
		Pipeline stencilPipeline{device};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		VkDescriptorSet descriptorSet;
	};
} // namespace Aspen