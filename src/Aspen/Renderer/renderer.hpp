#pragma once

#include "Aspen/Core/window.hpp"
#include "Aspen/Renderer/buffer.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/swap_chain.hpp"

namespace Aspen {
	// Mouse Picking rendering struct.
	struct MousePickingPass {
		VkFramebuffer frameBuffer{};
		SwapChain::FrameBufferAttachment color, depth{};
		VkRenderPass renderPass{};
	};

	class Renderer {
	public:
		Renderer(Window& window, Device& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&);
		Renderer(Renderer&&) = default;
		Renderer& operator=(Renderer&&) noexcept;

		VkRenderPass getPresentRenderPass() const {
			return swapChain->getPresentRenderPass();
		}

		SwapChain::OffscreenPass getOffscreenPass() {
			return swapChain->getOffscreenPass();
		}

		SwapChain::OffscreenPass getDepthPrePass() {
			return swapChain->getDepthPrePass();
		}

		VkExtent2D getSwapChainExtent() {
			return swapChain->getSwapChainExtent();
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
		void beginRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRenderPass renderPass, VkRect2D scissorDimensions, std::array<VkClearValue, 2> clearValues);
		void beginPresentRenderPass(VkCommandBuffer commandBuffer);
		void beginOffscreenRenderPass(VkCommandBuffer commandBuffer);
		void beginDepthPrePassRenderPass(VkCommandBuffer commandBuffer);
		void endRenderPass(VkCommandBuffer commandBuffer) const;
		void recreateSwapChain();

		void createMousePickingPass(MousePickingPass& mousePickingRenderPass);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();

		Window& window;
		Device& device;
		std::unique_ptr<SwapChain> swapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex{};
		int currentFrameIndex{0};
		bool isFrameStarted{false};
	};
} // namespace Aspen