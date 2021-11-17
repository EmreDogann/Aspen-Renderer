#pragma once
#include "aspen_device.hpp"
#include "aspen_model.hpp"
#include "aspen_pipeline.hpp"
#include "aspen_swap_chain.hpp"
#include "aspen_window.hpp"

// std
#include <memory>
#include <vector>

namespace Aspen {
    class FirstApp {
    public:
        static constexpr int WIDTH = 1024;
        static constexpr int HEIGHT = 768;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp &) = delete;
        FirstApp &operator=(const FirstApp &);
        void run();

    private:
        void loadModels();
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();

        AspenWindow aspenWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
        AspenDevice aspenDevice{aspenWindow};
        AspenSwapChain aspenSwapChain{aspenDevice, aspenWindow.getExtent()};
        std::unique_ptr<AspenPipeline> aspenPipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<VkCommandBuffer> commandBuffers;
        std::unique_ptr<AspenModel> aspenModel;
    };
}