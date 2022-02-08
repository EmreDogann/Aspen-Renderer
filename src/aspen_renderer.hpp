#pragma once
#include "aspen_device.hpp"
#include "aspen_model.hpp"
#include "aspen_swap_chain.hpp"
#include "aspen_window.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Aspen {
    class AspenRenderer {
    public:
        AspenRenderer(AspenWindow &window, AspenDevice &aspenDevice);
        ~AspenRenderer();

        AspenRenderer(const AspenRenderer &) = delete;
        AspenRenderer &operator=(const AspenRenderer &);

        VkRenderPass getSwapChainRenderPass() const { return aspenSwapChain->getRenderPass(); }
        bool isFrameInProgress() const { return isFrameStarted; }

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
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void recreateSwapChain();

    private:
        void createCommandBuffers();
        void freeCommandBuffers();

        AspenWindow &aspenWindow;
        AspenDevice &aspenDevice;
        std::unique_ptr<AspenSwapChain> aspenSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
}