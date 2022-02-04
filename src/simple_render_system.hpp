#pragma once
#include "aspen_device.hpp"
#include "aspen_game_object.hpp"
#include "aspen_model.hpp"
#include "aspen_pipeline.hpp"

// std
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Aspen {
    class SimpleRenderSystem {
    public:
        SimpleRenderSystem(AspenDevice &device, VkRenderPass renderPass);
        ~SimpleRenderSystem();

        SimpleRenderSystem(const SimpleRenderSystem &) = delete;
        SimpleRenderSystem &operator=(const SimpleRenderSystem &);

        void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<AspenGameObject> &gameObjects);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        AspenDevice &aspenDevice;

        std::unique_ptr<AspenPipeline> aspenPipeline;
        VkPipelineLayout pipelineLayout;
    };
}