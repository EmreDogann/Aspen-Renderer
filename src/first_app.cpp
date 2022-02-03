#include "first_app.hpp"
#include <GLFW/glfw3.h>

// std
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Aspen {
    FirstApp::FirstApp() {
        loadModels();
        createPipelineLayout();
        recreateSwapChain();
        createCommandBuffers();
    }

    FirstApp::~FirstApp() {
        vkDestroyPipelineLayout(aspenDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run() {
        // Subscribe lambda function to recreate swapchain when resizing window and draw a frame with the updated swapchain. This is done to enable smooth resizing.
        aspenWindow.windowResizeSubscribe([this]() {
            this->aspenWindow.resetWindowResizedFlag();
            this->recreateSwapChain();
            this->drawFrame();
        });

        // Game Loop
        while (!aspenWindow.shouldClose()) {
            glfwPollEvents(); // Process window level events (such as keystrokes).
            drawFrame();
        }

        vkDeviceWaitIdle(aspenDevice.device()); // Block CPU until all GPU operations have completed.
    }

    void FirstApp::loadModels() {
        std::vector<AspenModel::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        };

        // vertices = sierpinskiTriangle(vertices, 5);

        aspenModel = std::make_unique<AspenModel>(aspenDevice, vertices);
    }

    std::vector<AspenModel::Vertex> FirstApp::sierpinskiTriangle(std::vector<AspenModel::Vertex> vertices, const uint32_t depth) {
        // Base case.
        if (depth == 0) {
            return vertices;
        }

        std::vector<AspenModel::Vertex> newVertices;
        uint32_t counter = 0;
        // Get middle vertices.
        for (auto it = vertices.begin(); it != vertices.end(); ++it, ++counter) {
            newVertices.push_back(AspenModel::Vertex{it->position});
            newVertices.push_back({{(it->position.x + vertices[(counter + 1) % vertices.size()].position.x) / 2,
                                    (it->position.y + vertices[(counter + 1) % vertices.size()].position.y) / 2}});
            newVertices.push_back({{(it->position.x + vertices[(counter + 2) % vertices.size()].position.x) / 2,
                                    (it->position.y + vertices[(counter + 2) % vertices.size()].position.y) / 2}});
        }

        std::vector<AspenModel::Vertex> finalVertices;
        // Recursively compute smaller triangles.
        for (uint32_t i = 0; i < newVertices.size(); i += 3) {
            std::vector<AspenModel::Vertex> triangle{newVertices[i], newVertices[i + 1], newVertices[i + 2]};
            auto result = sierpinskiTriangle(triangle, depth - 1);
            finalVertices.insert(finalVertices.end(), result.begin(), result.end());
        }

        return finalVertices;
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
        AspenPipeline::defaultPipelineConfigInfo(pipelineConfig, aspenSwapChain->width(), aspenSwapChain->height());
        pipelineConfig.renderPass = aspenSwapChain->getRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        aspenPipeline = std::make_unique<AspenPipeline>(aspenDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
    }

    void FirstApp::createCommandBuffers() {
        // For now, keep a 1:1 relationship between the number of command buffers and the number of swap chains.
        commandBuffers.resize(aspenSwapChain->imageCount());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        // Primary Command Buffers (VK_COMMAND_BUFFER_LEVEL_PRIMARY) can be submitted to a queue to then be executed by the GPU. However, it cannot be called by other command buffers.
        // Secondary Command Buffers (VK_COMMAND_BUFFER_LEVEL_SECONDARY) cannot be submitted to a queue for execution. But, it can be called by other command buffers.
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = aspenDevice.getCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(aspenDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers.");
        }
    }

    void FirstApp::recordCommandBuffer(int imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer.");
        }

        // 1st Command: Begin render pass.
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = aspenSwapChain->getRenderPass();
        renderPassInfo.framebuffer = aspenSwapChain->getFrameBuffer(imageIndex); // Which frame buffer is this render pass is writing to?

        // Defines the area in which the shader loads and stores will take place.
        renderPassInfo.renderArea.offset = {0, 0};
        // Here we specify the swap chain extend and not the window extent because for high density displays (e.g. Apple's Retina displays), the size of the window will not be 1:1 with the size of the swap chain.
        renderPassInfo.renderArea.extent = aspenSwapChain->getSwapChainExtent();

        // What inital values we want our frame buffer attatchments to be cleared to.
        // This corresponds to how we've structured our render pass: Index 0 = color attatchment, Index 1 = Depth Attatchment.
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f}; // RGBA
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // Start recording commands to command buffer.
        // VK_SUBPASS_CONTENTS_INLINE signifies that all the commands we want to execute will be embedded directly into this primary command buffer.
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS signifies that all the commands we want to execute will come from secondary command buffers.
        // This means we cannot mix command types and have a primary command buffer that has both inline commands and secondary command buffers.
        vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        aspenPipeline->bind(commandBuffers[imageIndex]);
        aspenModel->bind(commandBuffers[imageIndex]);
        aspenModel->draw(commandBuffers[imageIndex]);

        // Stop recording.
        vkCmdEndRenderPass(commandBuffers[imageIndex]);
        if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer.");
        }
    }

    void FirstApp::recreateSwapChain() {
        auto extent = aspenWindow.getExtent();
        while (extent.width == 0 || extent.height == 0) {
            extent = aspenWindow.getExtent();
            glfwWaitEvents(); // While one of the windows dimensions is 0 (e.g. during minimization), wait until otherwise.
        }

        vkDeviceWaitIdle(aspenDevice.device());                                 // Wait until the current swapchain is no longer being used.
        aspenSwapChain = nullptr;                                               // Temporary fix as some systems do not allow the existsence of more than one swapchain. So by setting this smart point to null it will automatically destroy this swapchain.
        aspenSwapChain = std::make_unique<AspenSwapChain>(aspenDevice, extent); // Create new swapchain with new extents.
        createPipeline();                                                       // Pipeline can also depend on the swapchain, so we have to create that here as well.
    }

    void FirstApp::drawFrame() {
        uint32_t imageIndex;
        auto result = aspenSwapChain->acquireNextImage(&imageIndex); // Get the index of the frame buffer to render to next.

        // Recreate swapchain if window was resized.
        // VK_ERROR_OUT_OF_DATE_KHR occurs when the surface is no longer compatible with the swapchain (e.g. after window is resized).
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }

        // VK_SUBOPTIMAL_KHR may be returned if the swapchain no longer matches the surface properties exactly (e.g. if the window was resized).
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image.");
        }

        recordCommandBuffer(imageIndex);
        // Vulkan is going to go off and execute the commands in this command buffer to output that information to the selected frame buffer.
        result = aspenSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

        // Check again if window was resized during command buffer recording/submitting and recreate swapchain if so.
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || aspenWindow.wasWindowResized()) {
            aspenWindow.resetWindowResizedFlag();
            recreateSwapChain();
            return;
        }

        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image.");
        }
    }
}