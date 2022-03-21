#include "Aspen/Renderer/swap_chain.hpp"

namespace Aspen {

	SwapChain::SwapChain(Device& deviceRef, VkExtent2D extent)
	    : device{deviceRef}, windowExtent{extent} {
		init();
	}

	SwapChain::SwapChain(Device& deviceRef, VkExtent2D extent, std::shared_ptr<SwapChain> previous)
	    : device{deviceRef}, windowExtent{extent}, oldSwapChain{std::move(previous)} {
		init();

		// Clean up old swap chain since it's no longer being used. Setting to nullptr will signify the system to release its resources.
		oldSwapChain = nullptr;
	}

	void SwapChain::init() {
		createSwapChain();
		createImageViews();
		createSamplers();
		createRenderPasses();
		createDepthResources();
		createFramebuffers();
		createSyncObjects();
	}

	SwapChain::~SwapChain() {
		// Release memory of image views.
		for (auto* imageView : swapChainImageViews) {
			vkDestroyImageView(device.device(), imageView, nullptr);
		}
		swapChainImageViews.clear(); // Empty the array storing the image views.

		// Destroy the swap chain image and set the variable to nullptr to free its
		// system memory.
		if (swapChain != nullptr) {
			vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
			swapChain = nullptr;
		}

		// Destroy all depth image views.
		for (int i = 0; i < depthImages.size(); i++) {
			vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
			vkDestroyImage(device.device(), depthImages[i], nullptr);
			vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
		}

		// Destroy all framebuffer objects.
		for (auto* framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
		}

		// Destroy the present render pass.
		vkDestroyRenderPass(device.device(), presentRenderPass, nullptr);

		/*
		    Destroy offscreen rendering struct.
		*/

		// Color Attachment
		vkDestroyImageView(device.device(), offscreenPass.color.view, nullptr);
		vkDestroyImage(device.device(), offscreenPass.color.image, nullptr);
		vkFreeMemory(device.device(), offscreenPass.color.memory, nullptr);

		// Depth Attachment
		vkDestroyImageView(device.device(), offscreenPass.depth.view, nullptr);
		vkDestroyImage(device.device(), offscreenPass.depth.image, nullptr);
		vkFreeMemory(device.device(), offscreenPass.depth.memory, nullptr);

		// Framebuffer
		vkDestroyFramebuffer(device.device(), offscreenPass.frameBuffer, nullptr);

		// Offscreen renderpass
		vkDestroyRenderPass(device.device(), offscreenPass.renderPass, nullptr);

		// Sampler
		vkDestroySampler(device.device(), offscreenPass.sampler, nullptr);

		// Cleanup synchronization objects.
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device.device(), inFlightFences[i], nullptr);
		}
	}

	// Get the next available image in the swap chain to use for rendering operations.
	VkResult SwapChain::acquireNextImage(uint32_t* imageIndex) {
		vkWaitForFences(device.device(), 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		VkResult result =
		    vkAcquireNextImageKHR(device.device(),
		                          swapChain,
		                          std::numeric_limits<uint64_t>::max(),   // The max amount of time to wait in nanoseconds for an image to become available. Max of uint64_t disables this timeout.
		                          imageAvailableSemaphores[currentFrame], // Semaphore to be signaled when the image is ready to be rendered to. Must be a not signaled semaphore.
		                          VK_NULL_HANDLE,                         // Can also specify a fence if required.
		                          imageIndex);                            // The index of the swap chain image that has become available.

		return result;
	}

	VkResult SwapChain::submitCommandBuffers(const VkCommandBuffer* buffers, const uint32_t* imageIndex) {
		// If the current image is in flight, wait for that image's fence to be signaled so we don't send more frames than desired.
		if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
			// std::cout << "waiting for fence of image " << imageIndex << std::endl;
			vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX); // VK_TRUE means it will wait for all fences in the array to be signaled.
		}
		// Associate an available fence to the current image.
		imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// Specify the semaphore to wait on before execution begins and in which stage of the pipeline to wait.
		// In this case, we are saying to wait on semaphore x in the pipeline stage where writing to the image is performed until the image is ready.
		// This theoretically means that we can still perform other stages of the pipeline such as the vertex shader while the image is not yet available.
		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// Specify which command buffer to submit for execution.
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;

		// Specify the semaphore to signal once execution of the command buffer has completed.
		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device.device(), 1, &inFlightFences[currentFrame]); // Restore fences from signaled to unsignaled state.
		// Submit command buffer to the graphics queue.
		// We also pass in a fence to signal when the command buffer being submitted has finished executing.
		if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		// Specify which semaphores to wait on before presentation can happen.
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		// Specify which swap chain to present images to.
		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = imageIndex; // The index of the image to present.
		presentInfo.pResults = nullptr;         // Optional

		vkWaitForFences(device.device(), 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max()); // Prevents stuttering for some reason...
		// vkQueueWaitIdle(device.presentQueue());                               // Prevents stuttering for some reason...

		auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo); // Send image to be presented to the display.

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // Advance to the next frame.

		return result;
	}

	// Creates the swap chain.
	void SwapChain::createSwapChain() {
		// Get the supoprted swap chain details (format, present modes, extent).
		SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

		// Based on the supported swap chain details, pick the most suitable properties.
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// The minimum amount of swap chain images to function.
		// However, is it recommended to request one more image than the minimum as we may have to wait sometimes on the driver to complete internal operations.
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		// Ensure we do not exceed the maximum number of images. 0 means there is no limit.
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// Specify swap chain creation information.
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = device.surface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;

		// How many layers each image consists of.
		// Unless developing stereoscopic 3D applications, this is always 1.
		createInfo.imageArrayLayers = 1;

		// How will we use these images.
		// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT - Means they will be used to render directly to (so as a color attachment).
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// Get supported queue families.
		QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

		// Specify how swap chain images will be used across multiple queue families.
		// (e.g. how drawing on swap chain images from the graphics queue and then submitting them to the presentation queue).
		// VK_SHARING_MODE_EXCLUSIVE - An image is owned by one queue family at a time and must be explicitly transferred before using it in another queue family. This offers the best performance.
		// VK_SHARING_MODE_CONCURRENT - Images can be used across multiple queue families without explicit ownership transfers.
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;     // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		// Specify a transform to be applied to swap chain images (e.g. 90 degrees clockwise rotation). Passing in the current transform simply specifies no transformation.
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		// Should the alpha channel be used for blending with other windows in the window system.
		// VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR - No blending.
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode; // The swap chain present mode we picked earlier.
		createInfo.clipped = VK_TRUE;         // Should pixels obscured by other windows be discarded? True gives better performance.

		// Reference to old swap chain is needed if a new one is to be created due to something like a window resize.
		createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

		if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain!");
		}

		// Get the handles to the swap chain images that were created.
		// We only specified a minimum number of images in the swap chain, so the implementation is allowed to create a swap chain with more.
		// That's why we'll first query the final number of images with vkGetSwapchainImagesKHR,
		// then resize the container and finally call it again to retrieve the handles.
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());

		// Store copy of the swap chain images' surface format and extent for use later on in the application.
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void SwapChain::createMousePickingResources(VkFramebuffer& frameBuffer, FrameBufferAttachment& depthAttachment, VkRenderPass& renderPass) {
		createMousePickingDepthAttachment(depthAttachment);
		createMousePickingRenderPass(renderPass);
		createMousePickingFrameBuffer(frameBuffer, depthAttachment, renderPass);
	}

	// Create image views for all swap chain images for use as color targets.
	void SwapChain::createImageViews() {
		swapChainImageViews.resize(swapChainImages.size()); // Resize to the number of swap chain images.

		// Iterate through every swap chain image and create an image view for it.
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createViewInfo{};
			createViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createViewInfo.image = swapChainImages[i];

			createViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // How should the image be interpreted? E.g. 1D, 2D, 3D textures and cube maps.
			createViewInfo.format = swapChainImageFormat;    // Uses the same surface format as the images.

			// How to swizzle the color channels around. For now, this is kept at their defaults.
			createViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// subresourceRange describe what the image's purpose is and which part should be accessed.
			// Here we are saying our images will be used as color targets without any mipmapping or multiple layers.
			// For stereoscopic 3D applications, you would have multiple layers setup for the swap chain and so you could create multiple image views for each
			// image representing the views for the left and right eyes by accessing different layers.
			createViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createViewInfo.subresourceRange.baseMipLevel = 0;
			createViewInfo.subresourceRange.levelCount = 1;
			createViewInfo.subresourceRange.baseArrayLayer = 0;
			createViewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device.device(), &createViewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create texture image view!");
			}
		}

		// Create offscreen color attachment.
		VkImageCreateInfo offscreenImageCreateInfo{};
		offscreenImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		offscreenImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		offscreenImageCreateInfo.format = swapChainImageFormat;
		offscreenImageCreateInfo.extent.width = swapChainExtent.width;
		offscreenImageCreateInfo.extent.height = swapChainExtent.height;
		offscreenImageCreateInfo.extent.depth = 1;
		offscreenImageCreateInfo.mipLevels = 1;
		offscreenImageCreateInfo.arrayLayers = 1;
		offscreenImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		offscreenImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		offscreenImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		offscreenImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		offscreenImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // We will sample directly from the color attachment.

		// Create image and allocate memory for it.
		device.createImageWithInfo(offscreenImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenPass.color.image, offscreenPass.color.memory);

		// Create image view for the color attachment created above.
		VkImageViewCreateInfo offscreenImageViewCreateInfo{};
		offscreenImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		offscreenImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		offscreenImageViewCreateInfo.format = swapChainImageFormat;
		offscreenImageViewCreateInfo.subresourceRange = {};
		offscreenImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		offscreenImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		offscreenImageViewCreateInfo.subresourceRange.levelCount = 1;
		offscreenImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		offscreenImageViewCreateInfo.subresourceRange.layerCount = 1;
		offscreenImageViewCreateInfo.image = offscreenPass.color.image;

		if (vkCreateImageView(device.device(), &offscreenImageViewCreateInfo, nullptr, &offscreenPass.color.view) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen texture image view!");
		}
	}

	void SwapChain::createSamplers() {
		/*
		    Create sampler for offscreen rendering.
		*/
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 1.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		if (vkCreateSampler(device.device(), &samplerCreateInfo, nullptr, &offscreenPass.sampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen sampler!");
		}
	}

	// Setup the contents of the renderpass and a subpass.
	// Right now, there is one depth attachment and one color attachment.
	void SwapChain::createRenderPasses() {
		// Main Render Pass
		{
			// Define the color attachment.
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = getSwapChainImageFormat(); // Use the same surface format as the swap chain images.
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;    // We are not doing any multi-sampling yet so just stick to one.

			// loadOp and storeOp determine what to do with the data in the attachment before and after rendering.
			// VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment.
			// VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start.
			// VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them.
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

			// VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later.
			// VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation.
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			// Comments above work here as well but in this case it's for a stencil buffer.
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

			// initialLayout specifies which layout the image will have before the render pass begins.
			// VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image was in.
			// However, this means that the contents of the image are not guaranteed to be preserved, but that doesn't matter since we're going to clear it anyway.
			// finalLayout specifies the layout to automatically transition to when the render pass finishes.
			// We want the image to be ready for presentation using the swap chain after rendering, which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout.
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			// Attachment references to be used by subpasses.
			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0; // Which attachment to reference by its index in the attachment descriptions array.

			// What layout we would like the attachment to have during a subpass. We are going to use it as a color buffer so VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL will give the best performance.
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Check comments above.
			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = findDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef{};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Describe the subpass.
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // This subpass will be used for graphics. In the future Vulkan might support compute.
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			// Specify memory and execution dependencies between subpasses.
			VkSubpassDependency dependency = {};
			// Refers to the implicit subpass before the render pass. Or render passes BEFORE the current render pass.
			// If this was defined for dstSubpass then it would refer to the implicit subpass after the render pass. Or the render passes AFTER the current render pass.
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0; // Index of the subpass.

			// Wait for the swap chain to finish reading from the image before we access it.
			dependency.srcAccessMask = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			// What we have defined above is a dependency which states that the dstSubpass depends on the srcSubpass.
			// Subpasses don't execute in any particular order (for performance reasons) so we need dependencies between them to enforce a particular order.
			// Here we are saying, you can execute everything in the dstSubpass except the color output stage and early depth testing at which point you have to wait
			// for the srcSubpass to finish its color output stage and early depth testing.
			// The access masks specify which types of memory access each subpass dependency will need.

			// Create a render pass object.
			std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &presentRenderPass) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create render pass!");
			}
		}

		// Offscreen Render Pass
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = swapChainImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = findDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef{};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Describe the subpass.
			VkSubpassDescription subpassDescription = {};
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.colorAttachmentCount = 1;
			subpassDescription.pColorAttachments = &colorAttachmentRef;
			subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

			// Specify memory and execution dependencies between subpasses.
			std::array<VkSubpassDependency, 2> dependencies = {};
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Create a render pass object.
			std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpassDescription;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();

			if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &offscreenPass.renderPass) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create offscreen render pass!");
			}
		}
	}

	// Create a framebuffer object for every image in the swap chain.
	void SwapChain::createFramebuffers() {
		swapChainFramebuffers.resize(imageCount());
		for (size_t i = 0; i < imageCount(); i++) {
			std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageViews[i]};

			VkExtent2D swapChainExtent = getSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = presentRenderPass; // You can only use a framebuffer with the render passes that it is compatible with (roughly means the same number of type of attachments).
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1; // Number of layers in the image array.

			if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}

		/*
		    Create framebuffer for offscreen rendering.
		*/
		VkImageView attachments[2];
		attachments[0] = offscreenPass.color.view;
		attachments[1] = offscreenPass.depth.view;

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = offscreenPass.renderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = swapChainExtent.width;
		framebufferCreateInfo.height = swapChainExtent.height;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(device.device(), &framebufferCreateInfo, nullptr, &offscreenPass.frameBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen framebuffer!");
		}

		// TODO: Move this into its own function.
		// Fill a descriptor for later use in a descriptor set
		offscreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		offscreenPass.descriptor.imageView = offscreenPass.color.view;
		offscreenPass.descriptor.sampler = offscreenPass.sampler;
	}

	void SwapChain::createDepthResources() {
		VkFormat depthFormat = findDepthFormat();
		swapChainDepthFormat = depthFormat;
		VkExtent2D swapChainExtent = getSwapChainExtent();

		depthImages.resize(imageCount());
		depthImageMemorys.resize(imageCount());
		depthImageViews.resize(imageCount());

		for (int i = 0; i < depthImages.size(); i++) {
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImages[i], depthImageMemorys[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = depthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create depth image view!");
			}
		}

		// Create offscreen depth attachment.
		VkImageCreateInfo offscreenDepthImageCreateInfo{};
		offscreenDepthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		offscreenDepthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		offscreenDepthImageCreateInfo.format = depthFormat;
		offscreenDepthImageCreateInfo.extent.width = swapChainExtent.width;
		offscreenDepthImageCreateInfo.extent.height = swapChainExtent.height;
		offscreenDepthImageCreateInfo.extent.depth = 1;
		offscreenDepthImageCreateInfo.mipLevels = 1;
		offscreenDepthImageCreateInfo.arrayLayers = 1;
		offscreenDepthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		offscreenDepthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		offscreenDepthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		offscreenDepthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		offscreenDepthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		// Create image and allocate memory for it.
		device.createImageWithInfo(offscreenDepthImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenPass.depth.image, offscreenPass.depth.memory);

		// Create image view for the color attachment created above.
		VkImageViewCreateInfo offscreenDepthImageViewCreateInfo{};
		offscreenDepthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		offscreenDepthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		offscreenDepthImageViewCreateInfo.format = depthFormat;
		offscreenDepthImageViewCreateInfo.subresourceRange = {};
		offscreenDepthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // TODO: Do we need stencil bit flag here?
		offscreenDepthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		offscreenDepthImageViewCreateInfo.subresourceRange.levelCount = 1;
		offscreenDepthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		offscreenDepthImageViewCreateInfo.subresourceRange.layerCount = 1;
		offscreenDepthImageViewCreateInfo.image = offscreenPass.depth.image;

		if (vkCreateImageView(device.device(), &offscreenDepthImageViewCreateInfo, nullptr, &offscreenPass.depth.view) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen depth image view!");
		}
	}

	void SwapChain::createMousePickingDepthAttachment(FrameBufferAttachment& depthAttachment) {
		VkFormat depthFormat = findDepthFormat();

		// Create depth attachment.
		VkImageCreateInfo offscreenDepthImageCreateInfo{};
		offscreenDepthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		offscreenDepthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		offscreenDepthImageCreateInfo.format = depthFormat;
		offscreenDepthImageCreateInfo.extent.width = swapChainExtent.width;
		offscreenDepthImageCreateInfo.extent.height = swapChainExtent.height;
		offscreenDepthImageCreateInfo.extent.depth = 1;
		offscreenDepthImageCreateInfo.mipLevels = 1;
		offscreenDepthImageCreateInfo.arrayLayers = 1;
		offscreenDepthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		offscreenDepthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		offscreenDepthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		offscreenDepthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		offscreenDepthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		// Create image and allocate memory for it.
		device.createImageWithInfo(offscreenDepthImageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthAttachment.image, depthAttachment.memory);

		// Create image view for the color attachment created above.
		VkImageViewCreateInfo offscreenDepthImageViewCreateInfo{};
		offscreenDepthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		offscreenDepthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		offscreenDepthImageViewCreateInfo.format = depthFormat;
		offscreenDepthImageViewCreateInfo.subresourceRange = {};
		offscreenDepthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // TODO: Do we need stencil bit flag here?
		offscreenDepthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		offscreenDepthImageViewCreateInfo.subresourceRange.levelCount = 1;
		offscreenDepthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		offscreenDepthImageViewCreateInfo.subresourceRange.layerCount = 1;
		offscreenDepthImageViewCreateInfo.image = offscreenPass.depth.image;

		if (vkCreateImageView(device.device(), &offscreenDepthImageViewCreateInfo, nullptr, &depthAttachment.view) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Mouse Picking depth image view!");
		}
	}

	void SwapChain::createMousePickingRenderPass(VkRenderPass& renderPass) {
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 0;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Describe the subpass.
		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 0;
		subpassDescription.pColorAttachments = nullptr;
		subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

		// Specify memory and execution dependencies between subpasses.
		std::array<VkSubpassDependency, 1> dependencies = {};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		// Create a render pass object.
		std::array<VkAttachmentDescription, 1> attachments = {depthAttachment};
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen render pass!");
		}
	}

	void SwapChain::createMousePickingFrameBuffer(VkFramebuffer& frameBuffer, FrameBufferAttachment& depthAttachment, VkRenderPass& renderPass) {
		std::array<VkImageView, 1> attachments = {depthAttachment.view};

		VkExtent2D swapChainExtent = getSwapChainExtent();
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass; // You can only use a framebuffer with the render passes that it is compatible with (roughly means the same number of type of attachments).
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1; // Number of layers in the image array.

		if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create mouse picking framebuffer!");
		}
	}

	// Create semaphores and fences for every frame.
	void SwapChain::createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start all fences as signaled.

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			    vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			    vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create synchronization objects for a frame!");
			}
		}
	}

	// Select a swap surface with the desired format and color space.
	VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		// Find a swap surface format which is SRGB and supports the SRGB color space.
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		// If a suitable swap surface format was not found, then just return the first
		// one.
		return availableFormats[0];
	}

	// Select a suitable present mode in order of priority in the following order:
	// 1. VK_PRESENT_MODE_MAILBOX_KHR
	// 2. VK_PRESENT_MODE_IMMEDIATE_KHR
	// 3. VK_PRESENT_MODE_FIFO_KHR - This present mode is always guarenteed to be
	// available so is the safe pick.
	VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		// for (const auto& availablePresentMode : availablePresentModes) {
		// 	if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
		// 		// std::cout << "Present mode: Mailbox" << std::endl;
		// 		return availablePresentMode;
		// 	}
		// }

		// for (const auto &availablePresentMode : availablePresentModes) {
		//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
		//     std::cout << "Present mode: Immediate" << std::endl;
		//     return availablePresentMode;
		//   }
		// }

		std::cout << "Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// Select the pixel resolution of the swap chain images.
	// Some high DPI displays (such as Apple's retina displays) do not have a 1:1
	// between screen coordinates and pixels so pixel resolutions must be used.
	VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		// Some window managers allow us to set a range of resolution for the swap
		// chain images. This is indicated by the window manage setting the
		// currentExtent width and height to the max value of uint32_t.
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			// If the window manager does not allow us a range of resolutions, use the
			// resolution it has set.
			return capabilities.currentExtent;
		} else {
			// If a range of resolutions are allowed, pick the one that best suits the
			// actual extent of the window. windowExtent is passed in at initilization
			// and is the windows resolution in pixels.
			VkExtent2D actualExtent = windowExtent;
			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	VkFormat SwapChain::findDepthFormat() {
		return device.findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

} // namespace Aspen
