#include "first_app.hpp"
#include <GLFW/glfw3.h>
#include <ostream>

// GLM Libs
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Aspen {
    struct SimplePushConstantData {
        glm::mat2 transform{1.f};
        glm::vec2 offset;
        alignas(16) glm::vec3 color;
    };

    FirstApp::FirstApp() {
        loadGameObjects();
        createPipelineLayout();
        createPipeline();
    }

    FirstApp::~FirstApp() {
        vkDestroyPipelineLayout(aspenDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run() {
        std::cout << "maxPushConstantSize = " << aspenDevice.properties.limits.maxPushConstantsSize << "\n";

        // Subscribe lambda function to recreate swapchain when resizing window and draw a frame with the updated swapchain. This is done to enable smooth resizing.
        aspenWindow.windowResizeSubscribe([this]() {
            this->aspenWindow.resetWindowResizedFlag();
            this->aspenRenderer.recreateSwapChain();
            this->createPipeline(); // Right now this is not required as the new render pass will be compatible with the old one but is put here for future proofing.

            if (auto commandBuffer = this->aspenRenderer.beginFrame()) {
                this->aspenRenderer.beginSwapChainRenderPass(commandBuffer);
                this->renderGameObjects(commandBuffer);
                this->aspenRenderer.endSwapChainRenderPass(commandBuffer);
                this->aspenRenderer.endFrame();
            }
        });

        // Game Loop
        while (!aspenWindow.shouldClose()) {
            glfwPollEvents(); // Process window level events (such as keystrokes).
            if (auto commandBuffer = aspenRenderer.beginFrame()) {
                aspenRenderer.beginSwapChainRenderPass(commandBuffer);
                renderGameObjects(commandBuffer);
                aspenRenderer.endSwapChainRenderPass(commandBuffer);
                aspenRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(aspenDevice.device()); // Block CPU until all GPU operations have completed.
    }

    void FirstApp::loadGameObjects() {
        // Vec2 Position, Vec3 Color
        std::vector<AspenModel::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        };

        // vertices = sierpinskiTriangle(vertices, 7);

        auto aspenModel = std::make_shared<AspenModel>(aspenDevice, vertices);

        // https://www.color-hex.com/color-palette/5361
        std::vector<glm::vec3> colors{
            {1.0f, 0.7f, 0.73f},
            {1.0f, 0.87f, 0.73f},
            {1.0f, 1.0f, 0.73f},
            {0.73f, 1.0f, 0.8f},
            {0.73, 0.88f, 1.0f}};

        // Need to gamma correct the colors otherwise they will turn out to be too bright.
        for (auto &color : colors) {
            color = glm::pow(color, glm::vec3{2.2f});
        }

        // Create 40 different triangle game objects with incrementally different scales, rotations, and colors.
        for (int i = 0; i < 40; i++) {
            auto triangle = AspenGameObject::createGameObject();
            triangle.model = aspenModel;
            triangle.transform2d.scale = glm::vec2(0.5f) + i * 0.025f;
            triangle.transform2d.rotation = i * glm::pi<float>() * 0.025f;
            triangle.color = colors[i % colors.size()];
            gameObjects.push_back(std::move(triangle));
        }
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
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Which shaders will have access to this push constant range?
        pushConstantRange.offset = 0;                                                             // To be used if you are using separate ranges for the vertex and fragment shaders.
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(aspenDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }
    }

    // Create the graphics pipeline defined in aspen_pipeline.cpp
    void FirstApp::createPipeline() {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

        PipelineConfigInfo pipelineConfig{};
        AspenPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = aspenRenderer.getSwapChainRenderPass();
        pipelineConfig.pipelineLayout = pipelineLayout;
        aspenPipeline = std::make_unique<AspenPipeline>(aspenDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
    }

    void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer) {
        // Update the game object properties.
        int i = 0;
        for (auto &obj : gameObjects) {
            i += 1;
            obj.transform2d.rotation =
                glm::mod<float>(obj.transform2d.rotation + 0.00001f * i, 2.0f * glm::pi<float>()); // Slowly rotate game objects.
        }

        // Render the game objects.
        aspenPipeline->bind(commandBuffer);

        for (auto &obj : gameObjects) {
            SimplePushConstantData push{};
            push.transform = obj.transform2d.mat2();
            push.offset = obj.transform2d.translation;
            push.color = obj.color;

            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }
}