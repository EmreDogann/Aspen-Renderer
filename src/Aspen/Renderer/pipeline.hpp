#pragma once

#include "Aspen/Core/buffer.hpp"
#include "Aspen/Renderer/device.hpp"

// Libs
#include <vulkan/vulkan_core.h>

namespace Aspen {
	// Data specifying how we want to configure our pipeline.
	struct PipelineConfigInfo {
		VkPipelineViewportStateCreateInfo viewportInfo{};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0; // This is an index, not a count.

		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo(PipelineConfigInfo&&) = delete;            // Move Constructor
		PipelineConfigInfo& operator=(PipelineConfigInfo&&) = delete; // Move Assignment Operator

		PipelineConfigInfo() = default;
		~PipelineConfigInfo() = default;
	};

	class AspenPipeline {
	public:
		AspenPipeline(AspenDevice& device, const std::string& vertFilepath, const std::string& fragFilepath);
		~AspenPipeline();

		AspenPipeline(const AspenPipeline&) = delete;
		AspenPipeline& operator=(const AspenPipeline&) = delete;

		AspenPipeline(AspenPipeline&&) = delete;            // Move Constructor
		AspenPipeline& operator=(AspenPipeline&&) = delete; // Move Assignment Operator

		void bind(VkCommandBuffer commandBuffer, VkPipeline& pipeline);
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		void createGraphicsPipeline(const PipelineConfigInfo& configInfo, VkPipeline& pipeline);

		VkPipeline& getPresentPipeline() {
			return presentGraphicsPipeline;
		}

		VkPipeline& getOffscreenPipeline() {
			return offscreenGraphicsPipeline;
		}

	private:
		static std::vector<char> readFile(const std::string& filepath);

		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		AspenDevice& aspenDevice;
		VkPipeline presentGraphicsPipeline{};
		VkPipeline offscreenGraphicsPipeline{};
		VkShaderModule vertShaderModule{};
		VkShaderModule fragShaderModule{};
	};
} // namespace Aspen