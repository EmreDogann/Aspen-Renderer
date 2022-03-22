#pragma once

#include "Aspen/Renderer/buffer.hpp"
#include "Aspen/Renderer/device.hpp"

// Libs
#include <vulkan/vulkan_core.h>

namespace Aspen {

	class SwapChain {
	public:
		// Framebuffer for offscreen rendering
		struct FrameBufferAttachment {
			VkImage image{};
			VkDeviceMemory memory{};
			VkImageView view{};
		};

		// Offscreen rendering struct.
		struct OffscreenPass {
			VkFramebuffer frameBuffer{};
			FrameBufferAttachment color, depth;
			VkRenderPass renderPass{};
			VkSampler sampler{};
			VkDescriptorImageInfo descriptor{};
		};

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		SwapChain(Device& deviceRef, VkExtent2D windowExtent);
		SwapChain(Device& deviceRef, VkExtent2D windowExtent, std::shared_ptr<SwapChain> previous);
		~SwapChain();

		SwapChain(const SwapChain&) = delete;            // Copy Constructor
		SwapChain& operator=(const SwapChain&) = delete; // Copy Assignment Operator

		SwapChain(SwapChain&&) = delete;            // Move Constructor
		SwapChain& operator=(SwapChain&&) = delete; // Move Assignment Operator

		VkFramebuffer getFrameBuffer(int index) {
			return swapChainFramebuffers[index];
		}

		VkRenderPass getPresentRenderPass() {
			return presentRenderPass;
		}

		OffscreenPass& getOffscreenPass() {
			return offscreenPass;
		}

		OffscreenPass& getDepthPrePass() {
			return depthPrePass;
		}

		VkImageView getImageView(int index) {
			return swapChainImageViews[index];
		}
		size_t imageCount() {
			return swapChainImages.size();
		}
		VkFormat getSwapChainImageFormat() {
			return swapChainImageFormat;
		}
		VkExtent2D getSwapChainExtent() {
			return swapChainExtent;
		}
		uint32_t width() const {
			return swapChainExtent.width;
		}
		uint32_t height() const {
			return swapChainExtent.height;
		}

		float extentAspectRatio() const {
			return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
		}
		VkFormat findDepthFormat();

		VkResult acquireNextImage(uint32_t* imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer* buffers, const uint32_t* imageIndex);

		void createMousePickingPass(VkFramebuffer& frameBuffer, FrameBufferAttachment& colorAttachment, FrameBufferAttachment& depthAttachment, VkRenderPass& renderPass);
		void createDepthPrePass();

		bool compareSwapFormats(const SwapChain& swapChain) const {
			return swapChain.swapChainDepthFormat == swapChainDepthFormat && swapChain.swapChainImageFormat == swapChainImageFormat;
		}

	private:
		void init();
		void createSwapChain();

		void createImageViews();
		void createDepthResources();
		void createSamplers();

		void createRenderPasses();
		void createFramebuffers();

		void createSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat swapChainImageFormat{};
		VkFormat swapChainDepthFormat{};
		VkExtent2D swapChainExtent{};

		std::vector<VkFramebuffer> swapChainFramebuffers{};
		VkRenderPass presentRenderPass{};

		std::vector<VkImage> depthImages{};
		std::vector<VkDeviceMemory> depthImageMemorys{};
		std::vector<VkImageView> depthImageViews{};
		std::vector<VkImage> swapChainImages{};
		std::vector<VkImageView> swapChainImageViews{};

		OffscreenPass offscreenPass;
		OffscreenPass depthPrePass;

		Device& device;
		VkExtent2D windowExtent{};

		VkSwapchainKHR swapChain{};
		std::shared_ptr<SwapChain> oldSwapChain{};

		std::vector<VkSemaphore> imageAvailableSemaphores{};
		std::vector<VkSemaphore> renderFinishedSemaphores{};
		std::vector<VkFence> inFlightFences{};
		std::vector<VkFence> imagesInFlight{};
		size_t currentFrame = 0;
	};

} // namespace Aspen
