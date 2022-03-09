#include "Aspen/Renderer/device.hpp"
#include <iterator>
#include <stdexcept>

namespace Aspen {

	// Local callback function
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	// Debug messenger constructor.
	VkResult
	CreateDebugUtilsMessengerEXT(VkInstance instance_, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
		if (func != nullptr) {
			return func(instance_, pCreateInfo, pAllocator, pDebugMessenger);
		}

		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	// Debug messenger destructor.
	void DestroyDebugUtilsMessengerEXT(VkInstance instance_, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
		if (func != nullptr) {
			func(instance_, debugMessenger, pAllocator);
		}
	}

	// 1. Initialize Vulkan and picking a physical device.
	// 2. Setup validation layers that will help us with debugging our Vulkan code.
	AspenDevice::AspenDevice(AspenWindow& window) : window{window} {
		// Create Vulkan Instance.
		createInstance();

		// Setup validation layers (a.k.a debugging). Should be disabled for release builds.
		setupDebugMessenger();

		// Connection between window and Vulkan's ability to display results.
		createSurface();

		// Pick physical GPU we will be using to run Vulkan (can be more than 1 GPU).
		pickPhysicalDevice();

		// Create a logical device object from the physical device that was picked.
		// This will also create the queues from the queue families specified.
		createLogicalDevice();

		// Setup command pool. Useful for command buffer allocations.
		createCommandPool();

		// Setup ImGui Descriptor Pools.
		initImGuiBackend();

		// Create synchronization objects.
		// createSyncObjects();

		// 64Mb of memory.
		// VkDeviceSize stagingBufferSize = sizeof(char) * 64000000;
		// // Create and allocate one large staging buffer.
		// createBuffer(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		// vkMapMemory(device_, stagingBufferMemory, 0, stagingBufferSize, 0, nullptr);
	}

	AspenDevice::~AspenDevice() {
		vkDestroySemaphore(device_, transferSemaphore_, nullptr);
		vkDestroyCommandPool(device_, graphicsCommandPool, nullptr);
		vkDestroyCommandPool(device_, transferCommandPool, nullptr);
		vkDestroyDescriptorPool(device_, ImGui_descriptorPool, nullptr); // Destroy ImGui's descriptor pool.
		vkDestroyDevice(device_, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance_, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance_, surface_, nullptr);
		vkDestroyInstance(instance_, nullptr);
	}

	// This is setting up the application runtime's connection with the Vulkan runtime.
	// Essentially, this is what allows our CPU to communicate with the GPU.
	void AspenDevice::createInstance() {
		// Chech all validation layers that are requested are available.
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		// Enter information about our application.
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "AspenRenderer App";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		// Fill out struct to tell the Vulkan driver which global extensions and validation layers we want to use.
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// Because Vulkan is a platform agnostic API, we need to tell GLFW to give us the required extensions to interface with the window system of the OS.
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// Enable Debug Validation layers if bool set to true.
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		// Try and create a Vulkan instance_ using the information we just filled out.
		if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create instance_!");
		}

		// Print the available and required extensions.
		hasGflwRequiredInstanceExtensions();
	}

	// Choose a suitable physical device.
	void AspenDevice::pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr); // Get the number of physical devices.

		if (deviceCount == 0) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}
		std::cout << "Device count: " << deviceCount << std::endl;

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data()); // Get handles to the physical devices if present.

		// Iterate through the list of devices and get the first device which matches our criteria.
		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice_ = device;
				break;
			}
		}

		// If a suitable device could not be found, throw an error and stop the application.
		if (physicalDevice_ == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU!");
		}

		// Get the properties of the selected physical device.
		vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
		std::cout << "Physical device: " << properties.deviceName << std::endl;
	}

	// Create a logical device object using the suitable physical device.
	void AspenDevice::createLogicalDevice() {
		// Get the suuported queue families.
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		// The queue families the application will be utilizing.
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.transferFamily, indices.presentFamily};

		// The queue priority will inform Vulkan how to influence the scheduling of command buffer execution.
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1; // Create one queue from each queue family.
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Specify the device features we will be using.
		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		// deviceFeatures.fillModeNonSolid = true;
		// deviceFeatures.wideLines = true;

		// Create logical device object.
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		// Enable device specific extensions (e.g. swap chain).
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// Not necessary anymore because device specific validation layers have been deprecated.
		// But will leave implemented for backwards-compatability purposes.
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
		vkGetDeviceQueue(device_, indices.transferFamily, 0, &transferQueue_);
		vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
	}

	// Create the command pool.
	void AspenDevice::createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

		// Create the graphics command pool.
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		// Each command pool can only allocate command buffers that are submitted to a single type of queue.
		// In this case, we've associated this command pool with the graphics queue, meaning it can only allocate command buffers that submit to the graphics queue.
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

		// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are re-recorded with new commands very often (may change memory allocation behavior).
		// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be re-recorded individually, without this flag they all have to be reset together.
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device_, &poolInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics command pool!");
		}

		// Create the transfer command pool.
		poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		if (vkCreateCommandPool(device_, &poolInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create transfer command pool!");
		}
	}

	// Create semaphores and fences for transfer operations.
	void AspenDevice::createSyncObjects() {
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &transferSemaphore_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create synchronization objects for transfer operations!");
		}
	}

	// Create window surface (will call glfw's window surface creation function).
	void AspenDevice::createSurface() {
		window.createWindowSurface(instance_, &surface_);
	}

	// Check several properties to see if the device is suitable for what we want to use the application for.
	bool AspenDevice::isDeviceSuitable(VkPhysicalDevice device) {
		// Get the types of queues that this device supports.
		QueueFamilyIndices indices = findQueueFamilies(device);

		// Get the extensions that this device supports.
		bool extensionsSupported = checkDeviceExtensionSupport(device);

		// If all required extensions are supported, check if the swap chain properties are good.
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		// Get the features that are supported by this physical device.
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	// Set the type of debug messages to display.
	void AspenDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
	}

	// Create the debug messenger.
	void AspenDevice::setupDebugMessenger() {
		if (!enableValidationLayers) {
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		if (CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("Failed to set up debug messenger!");
		}
	}

	// Check if all requested validation layers are available.
	bool AspenDevice::checkValidationLayerSupport() {

		// Grab the currently available layers.
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// Check to see if all layers are supported. Return false if not.
		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	// Get the required extensions for Vulkan to interface with the window system.
	// Optionally add validation debug layers to the list of extensions to use.
	std::vector<const char*> AspenDevice::getRequiredExtensions() const {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = nullptr;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	// Print out the available extensions from GLFW and which ones were required by the application.
	void AspenDevice::hasGflwRequiredInstanceExtensions() {
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "Available extensions:" << std::endl;
		std::unordered_set<std::string> available;
		for (const auto& extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
			available.insert(extension.extensionName);
		}

		std::cout << "Required extensions:" << std::endl;
		auto requiredExtensions = getRequiredExtensions();
		for (const auto& required : requiredExtensions) {
			std::cout << "\t" << required << std::endl;
			if (available.find(required) == available.end()) {
				throw std::runtime_error("Missing required glfw extension");
			}
		}
	}

	// Get the features that are supported by this physical device.
	bool AspenDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	// Look for queue families which supports our desired queue flags.
	QueueFamilyIndices AspenDevice::findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// Get the number of supported queue families.
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0) {
			throw std::runtime_error("Failed to find GPUs any queue families!");
		}

		// Get handles to the queue families.
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		// Look for a queue family that supports graphics and transfer and is capable of presenting to a window surface.
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
				indices.graphicsFamilyHasValue = true;
			} else if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
				indices.transferFamily = i;
				indices.transferFamilyHasValue = true;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
				indices.presentFamilyHasValue = true;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	// Check the swap chains properties.
	// E.g. min/max number of images in swap chain, min/max width and height of images
	// Surface formats (pixel format, color space, etc.)
	// Available presentation modes
	SwapChainSupportDetails AspenDevice::querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

		// Get number of supported surface formats.
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			// Get the support surface formats.
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
		}

		// Get number of supported present modes.
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			// Get the support present modes.
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
		}
		return details;
	}

	VkFormat AspenDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("Failed to find supported format!");
	}

	// Find the appropriate memory type based on the requirements of the buffer and our application.
	uint32_t AspenDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties); // Get the available types of memory offered by the physical device.

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			// Use the typeFilter bit field to find the index of the suitable memory type that matches the memory type our buffer needs.
			// Use properties to check if the current memory type is suitable for our application (e.g. visible to host, has host coherency, etc.).
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	// Create arbitrary buffers.
	void AspenDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;                             // size of the buffer in bytes.
		bufferInfo.usage = usage;                           // What the buffer will be used for.
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Read comments on sharingMode in the swap chain creation function.

		if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create vertex buffer!");
		}

		// Get the memory requirements for the buffer based on the buffer info specified.
		VkMemoryRequirements memRequirements{};
		vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

		// Specify the info necessary to allocate the appropriate amount of memory for the buffer.
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties); // Get the memory type suitable for our needs.

		// TODO: Device allocations are limited (as low as 4096 allocations),
		// and so a custom memory allocator is needed to split up the allocations among many different objects (e.g. VMA).
		// Allocate the memory for the buffer and return a handle.
		if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate vertex buffer memory!");
		}

		// If successful, we can then associate this memory with the buffer.
		vkBindBufferMemory(device_, buffer, bufferMemory, 0); // Last parameter is offset within the region of memory.
	}

	// Create a temporary command buffer for the purposes of copying from the staging buffer.
	VkCommandBuffer AspenDevice::beginSingleTimeCommandBuffers() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = graphicsCommandPool; // Use the trasnfer command pool to create the command buffer.
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = nullptr;
		vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// Start command buffer recording.
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	// End the temporary command buffer recording, submit it to the queue and wait for submission to complete, then free the command buffer.
	void AspenDevice::endSingleTimeCommandBuffers(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		// TODO: Synchronize queues and transfer ownership from one queue to another.
		// VkBufferMemoryBarrier bufferMemoryBarrier{};
		// bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		// bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		// bufferMemoryBarrier.dstAccessMask = 0;
		// bufferMemoryBarrier.srcQueueFamilyIndex = queueFamilyIndices.transferFamily;
		// bufferMemoryBarrier.dstQueueFamilyIndex = queueFamilyIndices.graphicsFamily;
		// bufferMemoryBarrier.buffer = dstBuffer;
		// bufferMemoryBarrier.offset = 0;
		// bufferMemoryBarrier.size = size;

		// vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		// submitInfo.signalSemaphoreCount = 1;
		// submitInfo.pSignalSemaphores = &transferSemaphore_;

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue_); // Wait for the transfer operation to finish.
		                                 // TODO: Use fences and vkWaitForFences in order to allow multiple tranfers simultaneously, instead of executing one at a time.

		vkFreeCommandBuffers(device_, graphicsCommandPool, 1, &commandBuffer);
	}

	// Copy the contents of the staging buffer to a device local buffer.
	void AspenDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommandBuffers();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommandBuffers(commandBuffer, dstBuffer, size);
	}

	void AspenDevice::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommandBuffers();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {width, height, 1};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		endSingleTimeCommandBuffers(commandBuffer);
	}

	void AspenDevice::createImageWithInfo(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device_, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate image memory!");
		}

		if (vkBindImageMemory(device_, image, imageMemory, 0) != VK_SUCCESS) {
			throw std::runtime_error("Failed to bind image memory!");
		}
	}

	// Initialize ImGui and pass in Vulkan handles.
	void AspenDevice::initImGuiBackend() {
		VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		                                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		                                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		                                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		                                    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		                                    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		                                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		                                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		                                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		                                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		                                    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000;
		poolInfo.poolSizeCount = std::size(poolSizes);
		poolInfo.pPoolSizes = poolSizes;

		if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &ImGui_descriptorPool)) {
			throw std::runtime_error("Failed to create descriptor pool for ImGui!");
		}
	}

} // namespace Aspen