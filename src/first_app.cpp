#include "first_app.hpp"
#include <GLFW/glfw3.h>

// std
#include <array>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Aspen {
    FirstApp::FirstApp() {
        loadModels();
        createPipelineLayout();
        createPipeline();
        createCommandBuffers();
    }

    FirstApp::~FirstApp() {
        vkDestroyPipelineLayout(aspenDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run() {

        while (!aspenWindow.shouldClose()) {
            glfwPollEvents(); // Process window level events.
            drawFrame();
        }

        vkDeviceWaitIdle(aspenDevice.device()); // Block CPU until all GPU operations have completed.
    }

    void FirstApp::loadModels() {
        std::vector<AspenModel::Vertex> vertices{
            {{0.0f, -0.5f}},
            {{0.5f, 0.5f}},
            {{-0.5f, 0.5f}},
        };

        aspenModel = std::make_unique<AspenModel>(aspenDevice, vertices);
    }

    // For now the pipeline layout is empty.
    void FirstApp::createPipelineLayout() {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(aspenDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }
    }

    // Create the graphics pipeline defined in aspen_pipeline.cpp
    void FirstApp::createPipeline() {
        PipelineConfigInfo pipelineConfig{};
        AspenPipeline::defaultPipelineConfigInfo(pipelineConfig, aspenSwapChain.width(), aspenSwapChain.height());
        pipelineConfig.renderPass = aspenSwapChain.getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        aspenPipeline = std::make_unique<AspenPipeline>(aspenDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
    }

    void FirstApp::createCommandBuffers() {
        // For now, keep a 1:1 relationship between the number of command buffers and the number of swap chains.
        commandBuffers.resize(aspenSwapChain.imageCount());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = aspenDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(aspenDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers.");
        }

        // Index represents the current frame/command buffer.
        for (int i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording command buffer.");
            }

            // 1st Command: Begin render pass.
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = aspenSwapChain.getRenderPass();
            renderPassInfo.framebuffer = aspenSwapChain.getFrameBuffer(i); // Which frame buffer is this render pass is writing to?

            // Defines the area in which the shader loads and stores will take place.
            renderPassInfo.renderArea.offset = {0, 0};
            // Here we specify the swap chain extend and not the window extent because for high density displays (e.g. Apple's Retina displays), the size of the window will not be 1:1 with the size of the swap chain.
            renderPassInfo.renderArea.extent = aspenSwapChain.getSwapChainExtent();

            // What inital values we want our frame buffer attatchments to be cleared to.
            // This corresponds to how we've structured our render pass: Index 0 = color attatchment, Index 1 = Depth Attatchment.
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f}; // RGBA
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            // Start recording commands to command buffer.
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            aspenPipeline->bind(commandBuffers[i]);
            aspenModel->bind(commandBuffers[i]);
            aspenModel->draw(commandBuffers[i]);

            // Stop recording.
            vkCmdEndRenderPass(commandBuffers[i]);
            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to record command buffer.");
            }
        }
    }

    void FirstApp::drawFrame() {
        uint32_t imageIndex;
        auto result = aspenSwapChain.acquireNextImage(&imageIndex); // Get the index of the frame buffer to render to next.

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image.");
        }

        // Vulkan is going to go off and execute the commands in this command buffer to output that information to the selected frame buffer.
        result = aspenSwapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image.");
        }
    }
}