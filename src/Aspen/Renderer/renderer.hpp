#pragma once

#include "Aspen/Renderer/swap_chain.hpp"
#include "Aspen/Renderer/frame_info.hpp"

namespace Aspen {
	class Renderer {
	public:
		Renderer(Window& window, Device& device, const int desiredPresentMode);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&);
		Renderer(Renderer&&) = default;
		Renderer& operator=(Renderer&&) noexcept;

		VkRenderPass getPresentRenderPass() const {
			return swapChain->getRenderPass();
		}

		VkExtent2D getSwapChainExtent() {
			return swapChain->getSwapChainExtent();
		}

		uint32_t getSwapChainImageCount() const {
			return swapChain->imageCount();
		}

		VkFormat getSwapChainImageFormat() {
			return swapChain->getSwapChainImageFormat();
		}

		int getSwapChainMaxImagesInFlight() const {
			return swapChain->MAX_FRAMES_IN_FLIGHT;
		}

		float getAspectRatio() const {
			return swapChain->extentAspectRatio();
		}

		int getDesiredPresentMode() const {
			return desiredPresentMode;
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

		void setDesiredPresentMode(int mode) {
			desiredPresentMode = mode;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginRenderPass(VkCommandBuffer commandBuffer, RenderInfo renderInfo);
		void beginPresentRenderPass(VkCommandBuffer commandBuffer);
		void endRenderPass(VkCommandBuffer commandBuffer) const;
		void recreateSwapChain();

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

		int desiredPresentMode = 1;
	};
} // namespace Aspen