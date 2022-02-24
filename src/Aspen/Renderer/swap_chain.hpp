#pragma once

#include "Aspen/Renderer/device.hpp"

// Libs
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Aspen {

	class AspenSwapChain {
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		AspenSwapChain(AspenDevice &deviceRef, VkExtent2D windowExtent);
		AspenSwapChain(AspenDevice &deviceRef, VkExtent2D windowExtent, std::shared_ptr<AspenSwapChain> previous);
		~AspenSwapChain();

		AspenSwapChain(const AspenSwapChain &) = delete;            // Copy Constructor
		AspenSwapChain &operator=(const AspenSwapChain &) = delete; // Copy Assignment Operator

		AspenSwapChain(AspenSwapChain &&) = delete;            // Move Constructor
		AspenSwapChain &operator=(AspenSwapChain &&) = delete; // Move Assignment Operator

		VkFramebuffer getFrameBuffer(int index) {
			return swapChainFramebuffers[index];
		}
		VkRenderPass getRenderPass() {
			return renderPass;
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

		VkResult acquireNextImage(uint32_t *imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer *buffers, const uint32_t *imageIndex);

		bool compareSwapFormats(const AspenSwapChain &swapChain) const {
			return swapChain.swapChainDepthFormat == swapChainDepthFormat && swapChain.swapChainImageFormat == swapChainImageFormat;
		}

	private:
		void init();
		void createSwapChain();
		void createImageViews();
		void createDepthResources();
		void createRenderPass();
		void createFramebuffers();
		void createSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

		VkFormat swapChainImageFormat{};
		VkFormat swapChainDepthFormat{};
		VkExtent2D swapChainExtent{};

		std::vector<VkFramebuffer> swapChainFramebuffers{};
		VkRenderPass renderPass{};

		std::vector<VkImage> depthImages{};
		std::vector<VkDeviceMemory> depthImageMemorys{};
		std::vector<VkImageView> depthImageViews{};
		std::vector<VkImage> swapChainImages{};
		std::vector<VkImageView> swapChainImageViews{};

		AspenDevice &device;
		VkExtent2D windowExtent{};

		VkSwapchainKHR swapChain{};
		std::shared_ptr<AspenSwapChain> oldSwapChain{};

		std::vector<VkSemaphore> imageAvailableSemaphores{};
		std::vector<VkSemaphore> renderFinishedSemaphores{};
		std::vector<VkFence> inFlightFences{};
		std::vector<VkFence> imagesInFlight{};
		size_t currentFrame = 0;
	};

} // namespace Aspen
