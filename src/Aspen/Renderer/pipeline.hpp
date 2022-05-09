#pragma once

#include "Aspen/Core/model.hpp"

namespace Aspen {
	// Data specifying how we want to configure our pipeline.
	struct PipelineConfigInfo {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

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

	// Data specifying how we want to configure our ray-tracing pipeline.
	struct RayTracingPipelineConfigInfo {
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
		uint32_t maxRecursionDepth;
		VkPipelineLayout pipelineLayout = nullptr;

		RayTracingPipelineConfigInfo(const RayTracingPipelineConfigInfo&) = delete;
		RayTracingPipelineConfigInfo& operator=(const RayTracingPipelineConfigInfo&) = delete;
		RayTracingPipelineConfigInfo(RayTracingPipelineConfigInfo&&) = delete;            // Move Constructor
		RayTracingPipelineConfigInfo& operator=(RayTracingPipelineConfigInfo&&) = delete; // Move Assignment Operator

		RayTracingPipelineConfigInfo() = default;
		~RayTracingPipelineConfigInfo() = default;
	};

	class Pipeline {
	public:
		Pipeline(Device& device)
		    : device(device), deviceProcedures(device.deviceProcedures()) {}
		~Pipeline() {
			for (auto& module : shaderModules) {
				vkDestroyShaderModule(device.device(), module, nullptr);
			}
			vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
			vkDestroyPipeline(device.device(), pipeline, nullptr);
		}

		Pipeline(const Pipeline&) = delete;            // Copy Constructor
		Pipeline& operator=(const Pipeline&) = delete; // Copy Assignment Operator

		Pipeline(Pipeline&&) = delete;            // Move Constructor
		Pipeline& operator=(Pipeline&&) = delete; // Move Assignment Operator

		void bind(VkCommandBuffer commandBuffer, VkPipeline& pipeline);
		VkPipelineShaderStageCreateInfo createShaderModule(const std::string& shaderFilepath, VkShaderStageFlagBits stageType, VkSpecializationInfo* specializationInfo = nullptr);
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, VkPushConstantRange& pushConstantRange);
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		void createGraphicsPipeline(const PipelineConfigInfo& configInfo, VkPipeline& pipeline);
		void createRayTracingPipeline(const RayTracingPipelineConfigInfo& configInfo, VkPipeline& pipeline);
		static void defaultRayTracingPipelineConfigInfo(RayTracingPipelineConfigInfo& configInfo);

		VkPipeline& getPipeline() {
			return pipeline;
		}

		VkPipelineLayout& getPipelineLayout() {
			return pipelineLayout;
		}

		std::vector<VkShaderModule>& getShaderModules() {
			return shaderModules;
		}

	private:
		std::vector<char> readFile(const std::string& filepath);

		Device& device;
		// Used for calling extension functions that were manually leaded in DeviceProcedures.
		DeviceProcedures& deviceProcedures;

		enum PipelineType {
			Graphics,
			RayTracing,
			Compute
		} pipelineType;

		VkPipeline pipeline{};
		VkPipelineLayout pipelineLayout{};
		std::vector<VkShaderModule> shaderModules{};
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
	};
} // namespace Aspen