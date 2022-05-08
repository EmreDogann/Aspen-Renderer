#pragma once
#include "pch.h"

#include "Aspen/Core/window.hpp"
#include "Aspen/Renderer/descriptors.hpp"
#include "Aspen/Renderer/tools.hpp"
#include "Aspen/Renderer/device_procedures.hpp"

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

	class Device {
	public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		explicit Device(Window& window);
		~Device();

		// Not copyable or movable
		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		Device(Device&&) = delete;
		Device& operator=(Device&&) = delete;

		VkCommandPool getGraphicsCommandPool() {
			return graphicsCommandPool;
		}
		VkCommandPool getTransferCommandPool() {
			return transferCommandPool;
		}

		VkInstance instance() {
			return instance_;
		}

		VkPhysicalDevice physicalDevice() {
			return physicalDevice_;
		}

		VkDevice device() {
			return device_;
		}
		VkDevice device() const {
			return device_;
		}

		DeviceProcedures& deviceProcedures() {
			return *deviceProcedures_;
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
			return querySwapChainSupport(physicalDevice_);
		}

		DescriptorPool& getDescriptorPool() {
			return *descriptorPool;
		}
		DescriptorPool& getDescriptorPoolImGui() {
			return *descriptorPoolImGui;
		}

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		QueueFamilyIndices findPhysicalQueueFamilies() {
			queueFamilyIndices = findQueueFamilies(physicalDevice_);
			return queueFamilyIndices;
		}
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// Buffer Helper Functions
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		VkCommandBuffer beginSingleTimeCommandBuffers();
		void endSingleTimeCommandBuffers(VkCommandBuffer commandBuffer, VkBuffer dstBuffer = VkBuffer{}, VkDeviceSize size = VkDeviceSize{});
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

		void createImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

		VkDeviceAddress getBufferDeviceAddress(VkBuffer buffer);

		VkPhysicalDeviceProperties properties{};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

		VkPhysicalDeviceFeatures enabledFeatures{};
		VkPhysicalDeviceVulkan12Features enabledVK12Features{};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingFeatures{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		VkPhysicalDeviceMultiviewFeaturesKHR enabledMultiviewFeatures{};

		std::shared_ptr<DeviceProcedures> deviceProcedures_;

	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();
		void createDescriptorPool();

		// helper functions
		bool isDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> getRequiredExtensions() const;
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		std::unordered_set<std::string> availableInstanceExtensions();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		void createSyncObjects();
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		void initImGuiBackend();

		VkInstance instance_{};
		VkDebugUtilsMessengerEXT debugMessenger{};
		VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
		Window& window;
		VkCommandPool graphicsCommandPool{};
		VkCommandPool transferCommandPool{};

		VkSemaphore transferSemaphore_{};

		VkDevice device_{};
		VkSurfaceKHR surface_{};
		VkQueue graphicsQueue_{};
		VkQueue transferQueue_{};
		VkQueue presentQueue_{};
		QueueFamilyIndices queueFamilyIndices{};

		std::unique_ptr<DescriptorPool> descriptorPool{};
		std::unique_ptr<DescriptorPool> descriptorPoolImGui{};

		const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
		const std::vector<const char*> deviceExtensions = {
		    VK_KHR_SWAPCHAIN_EXTENSION_NAME,

		    // For omnidirectional shadows
		    VK_KHR_MULTIVIEW_EXTENSION_NAME,

		    // Ray Tracing Extensions
		    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

		    // Required by VK_KHR_acceleration_structure
		    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,

		    // Required for VK_KHR_ray_tracing_pipeline
		    VK_KHR_SPIRV_1_4_EXTENSION_NAME,

		    // Required by VK_KHR_spirv_1_4
		    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
		};
		const std::vector<const char*> instanceExtensions = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
	};

} // namespace Aspen