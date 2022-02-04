#pragma once
#include "aspen_device.hpp"
#include "aspen_game_object.hpp"
#include "aspen_model.hpp"
#include "aspen_pipeline.hpp"
#include "aspen_renderer.hpp"
#include "aspen_window.hpp"

// std
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

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
        void loadGameObjects();
        std::vector<AspenModel::Vertex> sierpinskiTriangle(std::vector<AspenModel::Vertex> vertices, const uint32_t depth);
        void createPipelineLayout();
        void createPipeline();
        void renderGameObjects(VkCommandBuffer commandBuffer);

        AspenWindow aspenWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
        AspenDevice aspenDevice{aspenWindow};
        AspenRenderer aspenRenderer{aspenWindow, aspenDevice};

        std::unique_ptr<AspenPipeline> aspenPipeline;
        VkPipelineLayout pipelineLayout;
        std::vector<AspenGameObject> gameObjects;
    };
}