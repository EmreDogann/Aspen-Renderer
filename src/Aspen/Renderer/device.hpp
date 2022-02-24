#pragma once

// Libs
#include "vulkan/vulkan_core.h"

#include "Aspen/Core/window.hpp"

namespace Aspen {

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		uint32_t graphicsFamily;
		uint32_t transferFamily;
		uint32_t presentFamily;
		bool graphicsFamilyHasValue = false;
		bool transferFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool isComplete() const {
			return graphicsFamilyHasValue && transferFamilyHasValue && presentFamilyHasValue;
		}
	};

	class AspenDevice {
	public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		explicit AspenDevice(AspenWindow &window);
		~AspenDevice();

		// Not copyable or movable
		AspenDevice(const AspenDevice &) = delete;
		AspenDevice &operator=(const AspenDevice &) = delete;
		AspenDevice(AspenDevice &&) = delete;
		AspenDevice &operator=(AspenDevice &&) = delete;

		VkCommandPool getGraphicsCommandPool() {
			return graphicsCommandPool;
		}
		VkCommandPool getTransferCommandPool() {
			return transferCommandPool;
		}

		VkDevice device() {
			return device_;
		}

		VkSurfaceKHR surface() {
			return surface_;
		}

		VkQueue graphicsQueue() {
			return graphicsQueue_;
		}
		VkQueue transferQueue() {
			return transferQueue_;
		}
		VkQueue presentQueue() {
			return presentQueue_;
		}

		VkSemaphore transferSemaphore() {
			return transferSemaphore_;
		}

		SwapChainSupportDetails getSwapChainSupport() {
			return querySwapChainSupport(physicalDevice);
		}

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		QueueFamilyIndices findPhysicalQueueFamilies() {
			queueFamilyIndices = findQueueFamilies(physicalDevice);
			return queueFamilyIndices;
		}
		VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// Buffer Helper Functions
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
		VkCommandBuffer beginSingleTimeCommandBuffers();
		void endSingleTimeCommandBuffers(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

		void createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

		VkPhysicalDeviceProperties properties{};

	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();

		// helper functions
		bool isDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char *> getRequiredExtensions() const;
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
		void hasGflwRequiredInstanceExtensions();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		void createSyncObjects();
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		VkInstance instance{};
		VkDebugUtilsMessengerEXT debugMessenger{};
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		AspenWindow &window;
		VkCommandPool graphicsCommandPool{};
		VkCommandPool transferCommandPool{};

		VkSemaphore transferSemaphore_{};

		VkDevice device_{};
		VkSurfaceKHR surface_{};
		VkQueue graphicsQueue_{};
		VkQueue transferQueue_{};
		VkQueue presentQueue_{};
		QueueFamilyIndices queueFamilyIndices{};

		// VkBuffer stagingBuffer{};
		// VkDeviceMemory stagingBufferMemory{};

		const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
		const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};
	};

} // namespace Aspen