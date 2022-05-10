#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	struct StorageImage {
		VkDeviceMemory memory;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view;
		VkFormat format;
		uint32_t width;
		uint32_t height;
	};

	struct BLASInput {
		std::vector<VkAccelerationStructureGeometryKHR> ASGeometry;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> ASBuildOffsetInfo;
		VkBuildAccelerationStructureFlagsKHR ASFlags{};
	};

	// Wraps all data required for an acceleration structure
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
		uint64_t device_address;
		std::unique_ptr<Buffer> buffer;
	};

	struct BuildAccelerationStructure {
		VkAccelerationStructureBuildGeometryInfoKHR buildGeomInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
		VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo{};
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		AccelerationStructure accelerationStructure{};
		AccelerationStructure oldAccelerationStructure{};
	};

	// Push constant structure for the ray tracer
	struct PushConstantRay {
		glm::vec4 clearColor{};
		glm::vec3 lightPosition{};
		float lightIntensity;
	};

	class RayTracingRenderSystem {
	public:
		RayTracingRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayouts, std::shared_ptr<Framebuffer> resourcesDepthPrePass);
		~RayTracingRenderSystem();

		RayTracingRenderSystem(const RayTracingRenderSystem&) = delete;
		RayTracingRenderSystem& operator=(const RayTracingRenderSystem&) = delete;

		RayTracingRenderSystem(RayTracingRenderSystem&&) = delete;            // Move Constructor
		RayTracingRenderSystem& operator=(RayTracingRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo);
		void createResources();
		void updateResources();
		void createAccelerationStructures(std::shared_ptr<Scene>& scene);
		void copyToImage(VkCommandBuffer cmdBuffer, VkImage dstImage, VkImageLayout initialLayout, uint32_t width, uint32_t height);
		void assignTextures(Scene& scene);
		RenderInfo prepareRenderInfo();
		void onResize();

		VkDescriptorSet getCurrentDescriptorSet() {
			return rtDescriptorSet;
		}

		std::shared_ptr<Framebuffer> getResources() {
			return resources;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet(std::shared_ptr<Scene>& scene);
		void createPipelines();
		void createPipelineLayout(std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout);

		void createBLAS(std::shared_ptr<Scene>& scene);
		BLASInput objectToGeometry(std::shared_ptr<Scene>& scene, uint32_t vertexOffset, uint32_t indexOffset, MeshComponent& model);
		void cmdCreateBLAS(VkCommandBuffer cmdBuffer, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAS, VkDeviceAddress scratchAddress, VkQueryPool queryPool);
		void cmdCompactBLAS(VkCommandBuffer cmdBuffer, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAS, VkQueryPool queryPool);

		void createTLAS(std::shared_ptr<Scene>& scene);
		void buildTLAS(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, bool update = false);

		void createShaderBindingTable();

		Device& device;
		// Used for calling extension functions that were manually leaded in DeviceProcedures.
		DeviceProcedures& deviceProcedures;
		Renderer& renderer;
		std::shared_ptr<Framebuffer> resources;
		std::weak_ptr<Framebuffer> resourcesDepthPrePass;

		// Ray Tracing Pipeline
		Pipeline pipeline{device};

		// Shader binding Table
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> rtShaderGroups;
		std::unique_ptr<Buffer> rtSBTBuffer;
		VkStridedDeviceAddressRegionKHR rgenRegion{};
		VkStridedDeviceAddressRegionKHR missRegion{};
		VkStridedDeviceAddressRegionKHR hitRegion{};
		VkStridedDeviceAddressRegionKHR callRegion{};

		PushConstantRay pushConstantRay{};

		// Host data to take specialization constants from
		struct SpecializationData {
			// Sets the lighting model used in the fragment "uber" shader
			int numLights;
		} specializationData;

		StorageImage storage_image;

		// Acceleration Structures
		std::vector<AccelerationStructure> m_BLAS;
		AccelerationStructure m_TLAS;

		// Ray Tracing Descriptors
		std::unique_ptr<DescriptorSetLayout> rtDescriptorSetLayout{};
		VkDescriptorSet rtDescriptorSet;

		std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayouts;

		// Textures
		std::unique_ptr<DescriptorSetLayout>
		    textureDescriptorSetLayout{};
		std::vector<VkDescriptorSet> textureDescriptorSets;

		std::unique_ptr<Buffer> uboBuffer;
	};
} // namespace Aspen