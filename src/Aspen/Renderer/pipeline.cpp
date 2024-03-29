#include "Aspen/Renderer/pipeline.hpp"

namespace Aspen {
	VkPipelineShaderStageCreateInfo Pipeline::createShaderModule(const std::string& shaderFilepath, VkShaderStageFlagBits stageType, VkSpecializationInfo* specializationInfo) {
		auto shaderCode = readFile(shaderFilepath);

		VkShaderModule shaderModule{};
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		if (vkCreateShaderModule(device.device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module.");
		}

		VkPipelineShaderStageCreateInfo shaderStage;
		// Setup shader stage for vertex shader.
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stageType; // Tells Vulkan what type of shader this is.
		shaderStage.module = shaderModule;
		shaderStage.pName = "main"; // Name of our entry function in our shader.
		shaderStage.flags = 0;
		shaderStage.pNext = nullptr;

		// Specify values for shader constants.
		// This allows you to configue how to use a shader module by specifying different values for the constants used
		// in it. This is more efficient than configuring the shader using variables at render time as the compiler can
		// do optimizations such as eliminating if statements and loops that depend on these values.
		shaderStage.pSpecializationInfo = specializationInfo;

		shaderModules.push_back(shaderModule);
		shaderStages.push_back(shaderStage);

		return shaderStage;
	}

	std::vector<char> Pipeline::readFile(const std::string& filepath) {
		// std::string enginePath = ENGINE_DIR + filepath;
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

	void Pipeline::createPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, VkPushConstantRange& pushConstantRange) {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	void Pipeline::createGraphicsPipeline(const PipelineConfigInfo& configInfo, VkPipeline& pipeline) {
		assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline:: No pipelineLayout provided in configInfo");
		assert(configInfo.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline:: No renderPass provided in configInfo");

		pipelineType = PipelineType::Graphics;

		// Specify the binding & attribute information for the vertex shader.
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(configInfo.attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(configInfo.bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingDescriptions.data();

		// Finally, setup the complete graphics pipeline struct.
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size(); // vert and frag shaders.
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo; // Optional - Can use this to change pipeline functionality without having to recreate the pipeline.
		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass; // Index to the subpass to use.

		// Optional - Can be used for optimization.
		// Can be less expensive for a GPU to create a new graphics pipeline by deriving from an existing one.
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		// Create graphics pipeline.
		if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline");
		}
	}

	void Pipeline::createRayTracingPipeline(const RayTracingPipelineConfigInfo& configInfo, VkPipeline& pipeline) {
		assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create ray-tracing pipeline:: No pipelineLayout provided in configInfo");

		pipelineType = PipelineType::RayTracing;

		// Finally, setup the complete ray-tracing pipeline struct.
		VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineInfo.stageCount = shaderStages.size(); // all ray tracing stages (shaders).
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.groupCount = configInfo.shaderGroups.size();
		pipelineInfo.pGroups = configInfo.shaderGroups.data();
		pipelineInfo.maxPipelineRayRecursionDepth = configInfo.maxRecursionDepth;
		pipelineInfo.layout = configInfo.pipelineLayout;

		// Optional - Can be used for optimization.
		// Can be less expensive for a GPU to create a new graphics pipeline by deriving from an existing one.
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		// Create graphics pipeline.
		if (deviceProcedures.vkCreateRayTracingPipelinesKHR(device.device(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline");
		}
	}

	// Bind a command buffer to a graphics pipeline.
	void Pipeline::bind(VkCommandBuffer commandBuffer, VkPipeline& pipeline) {
		// VK_PIPELINE_BIND_POINT_GRAPHICS signals that this is a graphics pipeline we are binding this command buffer to.
		// VK_PIPELINE_BIND_POINT_COMPUTE signals that this is a compute pipeline.
		// VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR signals that this is a ray tracing pipeline.
		if (pipelineType == PipelineType::Graphics) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		} else if (pipelineType == PipelineType::RayTracing) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
		} else if (pipelineType == PipelineType::Compute) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
		}
	}

	void Pipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {

		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Triangle_List = Every 3 verticies are grouped into a triangle.
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;              // If set to VK_TRUE when using a strip topology, we can specify an index value in an index buffer to breakup a strip.

		// // Configure viewport - Describes the transformation between the pipeline output and our target image.
		// // Transform from range -1 to 1 to range 0 to 1.
		// configInfo.viewport.x = 0.0f;
		// configInfo.viewport.y = 0.0f;
		// configInfo.viewport.width = static_cast<float>(width);
		// configInfo.viewport.height = static_cast<float>(height);

		// // Used to transform the z component from our shaders.
		// configInfo.viewport.minDepth = 0.0f;
		// configInfo.viewport.maxDepth = 1.0f;

		// // Any pixels outside the scissor rectangle will be discarded, instead of squished like with
		// configInfo.viewport configInfo.scissor.offset = {0, 0}; configInfo.scissor.extent = {width, height};

		// Combine viewport and scissor into a single create state variable.
		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = nullptr;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = nullptr;

		// Setup the rasterizer.
		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;        // Clamps z component to the range 0-1.
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE; // Geometry never passes through the rasterizer stage. This disables any output to the frambuffer.
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL; // How do we want to draw the triangle?
		configInfo.rasterizationInfo.lineWidth = 1.0f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;     // Can be used to alter depth values by adding a constant value/biasing them.
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f;          // Optional
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;    // Optional

		// How the rasterizer handles the edges of geometry (a.k.a Anti-Aliasing).
		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.0f;          // Optional
		configInfo.multisampleInfo.pSampleMask = nullptr;            // Optional
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;      // Optional

		// Configure how we combine colors in our frame buffer.
		configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

		// Configues pipeline to expect a dynamic viewport and scissor to be provided later.
		configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;

		// How to interpret the vertex buffer data.
		// Bindings - The spacing between data and whether the data is per-vertex or per-instance.
		// Attribute Descriptions - Type of the attributes passed to the vertex shader, which binding to load them from and at which offset.
		configInfo.bindingDescriptions = Aspen::Model::getBindingDescriptions();
		configInfo.attributeDescriptions = Aspen::Model::getAttributeDescriptions();
	}

	void Pipeline::defaultRayTracingPipelineConfigInfo(RayTracingPipelineConfigInfo& configInfo) {
		configInfo.maxRecursionDepth = 1;
		// Can be expanded in the future...
	}
} // namespace Aspen