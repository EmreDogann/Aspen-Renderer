#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	class MousePickingRenderSystem {
	public:
		struct MousePickingStorageBuffer {
			alignas(16) int64_t objectId;
		};

		MousePickingRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout);
		~MousePickingRenderSystem();

		MousePickingRenderSystem(const MousePickingRenderSystem&) = delete;
		MousePickingRenderSystem& operator=(const MousePickingRenderSystem&) = delete;

		MousePickingRenderSystem(MousePickingRenderSystem&&) = delete;            // Move Constructor
		MousePickingRenderSystem& operator=(MousePickingRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo);
		void onResize();

		VkDescriptorSet getDescriptorSet() {
			return descriptorSet;
		}

		MousePickingPass& getMousePickingResources() {
			return mousePickingResources;
		}

		std::unique_ptr<Buffer>& getStorageBuffer() {
			return storageBuffer;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout);

		Device& device;
		Renderer& renderer;
		Pipeline pipeline{device};

		MousePickingPass mousePickingResources{};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		VkDescriptorSet descriptorSet;

		std::unique_ptr<Buffer> storageBuffer;
	};
} // namespace Aspen