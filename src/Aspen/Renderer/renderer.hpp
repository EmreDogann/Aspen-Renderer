#pragma once

#include "Aspen/Core/window.hpp"
#include "Aspen/Renderer/buffer.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/swap_chain.hpp"

namespace Aspen {
	class Renderer {
	public:
		Renderer(Window& window, Device& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&);
		Renderer(Renderer&&) = default;
		Renderer& operator=(Renderer&&) noexcept;

		VkFramebuffer getOffscreenFrameBuffer() {
			return swapChain->getOffscreenFrameBuffer();
		}

		VkRenderPass getPresentRenderPass() const {
			return swapChain->getPresentRenderPass();
		}
		VkRenderPass getOffscreenRenderPass() const {
			return swapChain->getOffscreenRenderPass();
		}

		SwapChain::OffscreenPass getOffscreenPass() {
			return swapChain->getOffscreenPass();
		}

		VkExtent2D getSwapChainExtent() {
			return swapChain->getSwapChainExtent();
		}

		VkDescriptorImageInfo& getOffscreenDescriptorInfo() const {
			return swapChain->getOffscreenDescriptorInfo();
		}

		uint32_t getSwapChainImageCount() const {
			return swapChain->imageCount();
		}

		int getSwapChainMaxImagesInFlight() const {
			return swapChain->MAX_FRAMES_IN_FLIGHT;
		}

		float getAspectRatio() const {
			return swapChain->extentAspectRatio();
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

		Window& window;
		Device& device;
		std::unique_ptr<SwapChain> swapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex{};
		int currentFrameIndex{0};
		bool isFrameStarted{false};
	};
} // namespace Aspen