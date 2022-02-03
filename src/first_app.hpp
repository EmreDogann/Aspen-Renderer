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
        std::vector<AspenModel::Vertex> sierpinskiTriangle(std::vector<AspenModel::Vertex> vertices, const uint32_t depth);
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();
        void recreateSwapChain();
        void recordCommandBuffer(int imageIndex);

        AspenWindow aspenWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
        AspenDevice aspenDevice{aspenWindow};
        std::unique_ptr<AspenSwapChain> aspenSwapChain;
        std::unique_ptr<AspenPipeline> aspenPipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<VkCommandBuffer> commandBuffers;
        std::unique_ptr<AspenModel> aspenModel;
    };
}