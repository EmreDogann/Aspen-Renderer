#include "aspen_pipeline.hpp"
#include "aspen_model.hpp"

// std
#include <cassert>
#include <corecrt.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Aspen {

    AspenPipeline::AspenPipeline(AspenDevice &device, const std::string &vertFilepath, const std::string &fragFilepath, const PipelineConfigInfo &configInfo) : aspenDevice(device) {
        createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
    }

    AspenPipeline::~AspenPipeline() {
        vkDestroyShaderModule(aspenDevice.device(), vertShaderModule, nullptr);
        vkDestroyShaderModule(aspenDevice.device(), fragShaderModule, nullptr);
        vkDestroyPipeline(aspenDevice.device(), graphicsPipeline, nullptr);
    }

    std::vector<char> AspenPipeline::readFile(const std::string &filepath) {
        // ate = Bit flag to make sure we seek to the end of a file when it is opened.
        // binary = Bit flag to set it to read in the file as a binary to prevent any unwanted text transformations.
        std::ifstream file{filepath, std::ios::ate | std::ios::binary};

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filepath);
        }

        // Get file size by just getting the last position of the file.
        // NOTE: This is not actually guaranteed to return the correct data as explained here: https://stackoverflow.com/questions/22984956/tellg-function-give-wrong-size-of-file/22986486#22986486
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        // Read entire file into a char vector the size of the file.
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    void AspenPipeline::createGraphicsPipeline(const std::string &vertFilepath, const std::string &fragFilepath, const PipelineConfigInfo &configInfo) {

        assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline:: No pipelineLayout provided in configInfo");
        assert(configInfo.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline:: No renderPass provided in configInfo");
        auto vertCode = readFile(vertFilepath);
        auto fragCode = readFile(fragFilepath);

        createShaderModule(vertCode, &vertShaderModule);
        createShaderModule(fragCode, &fragShaderModule);

        VkPipelineShaderStageCreateInfo shaderStages[2];
        // Setup shader stage for vertex shader.
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; // Tells Vulkan that this shader stage is for the vertex.
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "main"; // Name of our entry function in our shader.
        shaderStages[0].flags = 0;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;

        // Do the same for the fragment shader.
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "main";
        shaderStages[1].flags = 0;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        // How to interpret the vertex buffer data.
        auto bindingDescriptions = AspenModel::Vertex::getBindingDescriptions();
        auto attributeDescriptions = AspenModel::Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

        // Finally, setup the complete graphics pipeline struct.
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2; // vert and frag shaders.
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
        pipelineInfo.pViewportState = &configInfo.viewportInfo;
        pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
        pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
        pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
        pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
        pipelineInfo.pDynamicState = nullptr; // Optional - Can use this to change pipeline functionality without having to recreate the pipeline.

        pipelineInfo.layout = configInfo.pipelineLayout;
        pipelineInfo.renderPass = configInfo.renderPass;
        pipelineInfo.subpass = configInfo.subpass;

        // Optional - Can be used for optimization.
        // Can be less expensive for a GPU to create a new graphics pipeline by deriving from an existing one.
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        // Create graphics pipeline.
        if (vkCreateGraphicsPipelines(aspenDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }
    }

    void AspenPipeline::createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        if (vkCreateShaderModule(aspenDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module.");
        }
    }

    // Bind a command buffer to a graphics pipeline.
    void AspenPipeline::bind(VkCommandBuffer commandBuffer) {
        // VK_PIPELINE_BIND_POINT_GRAPHICS signals that this is a graphics pipeline we are binding this command buffer to.
        // VK_PIPELINE_BIND_POINT_COMPUTE signals that this is a compute pipeline.
        // VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR signals that this is a ray tracing pipeline.
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

    void AspenPipeline::defaultPipelineConfigInfo(PipelineConfigInfo &configInfo, uint32_t width, uint32_t height) {

        configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Triangle_List = Every 3 verticies are grouped into a triangle.
        configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;              // If set to VK_TRUE when using a strip type topology, we can specify a special index value in an index buffer in order to breakup a strip and create disconnected geometry.

        // Configure viewport - Describes the transformation between the pipeline output and our target image.
        // Transform from range -1 to 1 to range 0 to 1.
        configInfo.viewport.x = 0.0f;
        configInfo.viewport.y = 0.0f;
        configInfo.viewport.width = static_cast<float>(width);
        configInfo.viewport.height = static_cast<float>(height);

        // Used to transform the z component from our shaders.
        configInfo.viewport.minDepth = 0.0f;
        configInfo.viewport.maxDepth = 1.0f;

        // Any pixels outside the scissor rectangle will be discarded, instead of squished like with configInfo.viewport
        configInfo.scissor.offset = {0, 0};
        configInfo.scissor.extent = {width, height};

        // Combine viewport and scissor into a single create state variable.
        configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        configInfo.viewportInfo.viewportCount = 1;
        configInfo.viewportInfo.pViewports = &configInfo.viewport;
        configInfo.viewportInfo.scissorCount = 1;
        configInfo.viewportInfo.pScissors = &configInfo.scissor;

        // Setup the rasterizer.
        configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        configInfo.rasterizationInfo.depthClampEnable = VK_FALSE; // Clamps z component to the range 0-1.
        configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL; // How do we want to draw the triangle?
        configInfo.rasterizationInfo.lineWidth = 1.0f;
        configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
        configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
        configInfo.rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
        configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

        // How the rasterizer handles the edges of geomery (a.k.a Anti-Aliasing).
        configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
        configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        configInfo.multisampleInfo.minSampleShading = 1.0f;          // Optional
        configInfo.multisampleInfo.pSampleMask = nullptr;            // Optional
        configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
        configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

        // Configure how we combine colors in our frame buffer.
        configInfo.colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
        configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

        // Combine attachment into colorBlendInfo.
        configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
        configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
        configInfo.colorBlendInfo.attachmentCount = 1;
        configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
        configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
        configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
        configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
        configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // Optional

        // Attachment to our depth buffer - Stores a value for a every pixel
        configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
        configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
        configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.minDepthBounds = 0.0f; // Optional
        configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
        configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.front = {}; // Optional
        configInfo.depthStencilInfo.back = {};  // Optional
    }
}