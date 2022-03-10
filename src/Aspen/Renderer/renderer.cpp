#include "Aspen/Renderer/renderer.hpp"

namespace Aspen {

	AspenRenderer::AspenRenderer(AspenWindow& window, AspenDevice& device, Buffer& bufferManager) : aspenWindow{window}, aspenDevice{device}, bufferManager{bufferManager} {
		recreateSwapChain();
		createCommandBuffers();
	}

	AspenRenderer::~AspenRenderer() {
		freeCommandBuffers();
	}

	void AspenRenderer::createCommandBuffers() {
		commandBuffers.resize(AspenSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

		// Primary Command Buffers (VK_COMMAND_BUFFER_LEVEL_PRIMARY) can be submitted
		// to a queue to then be executed by the GPU. However, it cannot be called by
		// other command buffers. Secondary Command Buffers
		// (VK_COMMAND_BUFFER_LEVEL_SECONDARY) cannot be submitted to a queue for
		// execution. But, it can be called by other command buffers.
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = aspenDevice.getGraphicsCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(aspenDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers.");
		}
	}

	void AspenRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(aspenDevice.device(), aspenDevice.getGraphicsCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	void AspenRenderer::recreateSwapChain() {
		auto extent = aspenWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) { // While the window is minimized...
			extent = aspenWindow.getExtent();
			glfwWaitEvents(); // While one of the windows dimensions is 0 (e.g. during minimization), wait until otherwise.
		}

		vkDeviceWaitIdle(aspenDevice.device()); // Wait until the current swapchain is no longer being used.

		if (aspenSwapChain == nullptr) {
			aspenSwapChain = std::make_unique<AspenSwapChain>(aspenDevice, bufferManager, extent); // Create new swapchain with new extents.
		} else {
			std::shared_ptr<AspenSwapChain> oldSwapChain = std::move(aspenSwapChain);

			// Create new swapchain with new extents and pass through the old swap chain if it exists.
			aspenSwapChain = std::make_unique<AspenSwapChain>(aspenDevice, bufferManager, extent, oldSwapChain);

			// Check if the old and new swap chains are compatible.
			if (!oldSwapChain->compareSwapFormats(*aspenSwapChain)) {
				throw std::runtime_error("Swap chain image(or depth) format has changed!");
			}
		}

		// If Render Pass is compatible there is no need to create a new pipeline.
		// TODO: Check if new and old render passes are compatible.
	}

	// beginFrame will start recording the current command buffer and check that the current frame buffer is still valid.
	VkCommandBuffer AspenRenderer::beginFrame() {
		assert(!isFrameStarted && "Cannot call beginFrame while it is already in progress!");

		auto result = aspenSwapChain->acquireNextImage(&currentImageIndex); // Get the index of the frame buffer to render to next.

		// Recreate swapchain if window was resized.
		// VK_ERROR_OUT_OF_DATE_KHR occurs when the surface is no longer compatible with the swapchain (e.g. after window is resized).
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		// VK_SUBOPTIMAL_KHR may be returned if the swapchain no longer matches the
		// surface properties exactly (e.g. if the window was resized).
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image.");
		}

		isFrameStarted = true;

		// Start command buffer recording.
		auto* commandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;                  // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer.");
		}

		return commandBuffer;
	}

	// End from will stop recording the current command buffer and submit it to the render queue.
	void AspenRenderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress!");

		// Stop command buffer recording.
		auto* commandBuffer = getCurrentCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer.");
		}

		// Submit command buffer.
		// Vulkan is going to go off and execute the commands in this command buffer
		// to output that information to the selected frame buffer.
		auto result = aspenSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

		// Check again if window was resized during command buffer recording/submitting and recreate swapchain if so.
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || aspenWindow.wasWindowResized()) {
			aspenWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image.");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % AspenSwapChain::MAX_FRAMES_IN_FLIGHT; // Increment currentFrameIndex.
	}

	void AspenRenderer::beginPresentRenderPass(VkCommandBuffer commandBuffer) {
		beginRenderPass(commandBuffer, aspenSwapChain->getFrameBuffer(currentImageIndex), aspenSwapChain->getPresentRenderPass());
	}
	void AspenRenderer::beginOffscreenRenderPass(VkCommandBuffer commandBuffer) {
		beginRenderPass(commandBuffer, aspenSwapChain->getOffscreenFrameBuffer(), aspenSwapChain->getOffscreenRenderPass());
	}

	// beginSwapChainRenderPass will start the render pass in order to then record commands to it.
	void AspenRenderer::beginRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRenderPass renderPass) {
		assert(isFrameStarted && "Can't call beginRenderPass if frame is not in progress!");
		assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame.");

		// 1st Command: Begin render pass.
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer; // Which frame buffer is this render pass writing to?

		// Defines the area in which the shader loads and stores will take place.
		renderPassInfo.renderArea.offset = {0, 0};
		// Here we specify the swap chain extent and not the window extent because for
		// high density displays (e.g. Apple's Retina displays), the size of the
		// window will not be 1:1 with the size of the swap chain.
		renderPassInfo.renderArea.extent = aspenSwapChain->getSwapChainExtent();

		// What inital values we want our frame buffer attatchments to be cleared to.
		// This corresponds to how we've structured our render pass: Index 0 = color
		// attatchment, Index 1 = Depth Attatchment.
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f}; // RGBA
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// Begin the render pass instance and start recording commands to that render
		// pass. VK_SUBPASS_CONTENTS_INLINE signifies that all the commands we want to
		// execute will be embedded directly into this primary command buffer.
		// VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS signifies that all the
		// commands we want to execute will come from secondary command buffers. This
		// means we cannot mix command types and have a primary command buffer that
		// has both inline commands and secondary command buffers.
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Setup dynamic viewport and scissor.
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(aspenSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(aspenSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{{0, 0}, aspenSwapChain->getSwapChainExtent()};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	// End the render pass when commands have been recorded.
	void AspenRenderer::endRenderPass(VkCommandBuffer commandBuffer) const {
		assert(isFrameStarted && "Can't call endRenderPass if frame is not in progress!");
		assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame.");

		vkCmdEndRenderPass(commandBuffer);
	}
} // namespace Aspen