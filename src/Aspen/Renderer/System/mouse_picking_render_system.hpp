#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	class MousePickingRenderSystem {
	public:
		struct MousePickingStorageBuffer {
			alignas(16) int64_t objectId;
		};

		MousePickingRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout, std::shared_ptr<Framebuffer> resources);
		~MousePickingRenderSystem() = default;

		MousePickingRenderSystem(const MousePickingRenderSystem&) = delete;
		MousePickingRenderSystem& operator=(const MousePickingRenderSystem&) = delete;

		MousePickingRenderSystem(MousePickingRenderSystem&&) = delete;            // Move Constructor
		MousePickingRenderSystem& operator=(MousePickingRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo);
		void loadAttachment();
		RenderInfo prepareRenderInfo();
		void onResize();

		VkDescriptorSet getDescriptorSet() {
			return descriptorSet;
		}

		Framebuffer& getResources() {
			return *resources;
		}

		std::unique_ptr<Buffer>& getStorageBuffer() {
			return storageBuffer;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout();

		Device& device;
		Renderer& renderer;
		std::unique_ptr<Framebuffer> resources;
		std::weak_ptr<Framebuffer> resourcesDepthPrePass;
		Pipeline pipeline{device};

		DescriptorSetLayout& globalDescriptorSetLayout;
		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		VkDescriptorSet descriptorSet;

		std::unique_ptr<Buffer> storageBuffer;
	};
} // namespace Aspen