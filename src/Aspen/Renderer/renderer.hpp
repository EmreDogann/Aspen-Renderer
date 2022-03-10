#pragma once

#include "Aspen/Core/buffer.hpp"
#include "Aspen/Core/window.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/swap_chain.hpp"

// Libs
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

namespace Aspen {
	class AspenRenderer {
	public:
		AspenRenderer(AspenWindow& window, AspenDevice& aspenDevice, Buffer& bufferManager);
		~AspenRenderer();

		AspenRenderer(const AspenRenderer&) = delete;
		AspenRenderer& operator=(const AspenRenderer&);
		AspenRenderer(AspenRenderer&&) = default;
		AspenRenderer& operator=(AspenRenderer&&) noexcept;

		VkFramebuffer getOffscreenFrameBuffer() {
			return aspenSwapChain->getOffscreenFrameBuffer();
		}

		VkRenderPass getPresentRenderPass() const {
			return aspenSwapChain->getPresentRenderPass();
		}
		VkRenderPass getOffscreenRenderPass() const {
			return aspenSwapChain->getOffscreenRenderPass();
		}

		AspenSwapChain::OffscreenPass getOffscreenPass() {
			return aspenSwapChain->getOffscreenPass();
		}

		VkExtent2D getSwapChainExtent() {
			return aspenSwapChain->getSwapChainExtent();
		}

		VkDescriptorImageInfo& getOffscreenDescriptorInfo() const {
			return aspenSwapChain->getOffscreenDescriptorInfo();
		}

		uint32_t getSwapChainImageCount() const {
			return aspenSwapChain->imageCount();
		}

		int getSwapChainMaxImagesInFlight() const {
			return aspenSwapChain->MAX_FRAMES_IN_FLIGHT;
		}

		float getAspectRatio() const {
			return aspenSwapChain->extentAspectRatio();
		}

		bool isFrameInProgress() const {
			return isFrameStarted;
		}

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame is not in progress.");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame is not in progress.");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginPresentRenderPass(VkCommandBuffer commandBuffer);
		void beginOffscreenRenderPass(VkCommandBuffer commandBuffer);
		void endRenderPass(VkCommandBuffer commandBuffer) const;
		void recreateSwapChain();

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void beginRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRenderPass renderPass);

		AspenWindow& aspenWindow;
		AspenDevice& aspenDevice;
		Buffer& bufferManager;
		std::unique_ptr<AspenSwapChain> aspenSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex{};
		int currentFrameIndex{0};
		bool isFrameStarted{false};
	};
} // namespace Aspen