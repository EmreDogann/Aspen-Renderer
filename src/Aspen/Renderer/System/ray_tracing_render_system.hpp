#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
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
		VkAccelerationStructureBuildGeometryInfoKHR buildGeomInfo{};
		VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo{};
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo{};
		AccelerationStructure accelerationStructure{};
		AccelerationStructure oldAccelerationStructure{};
	};

	class RayTracingRenderSystem {
	public:
		RayTracingRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout, std::shared_ptr<Framebuffer> resourcesDepthPrePass);
		~RayTracingRenderSystem() = default;

		RayTracingRenderSystem(const RayTracingRenderSystem&) = delete;
		RayTracingRenderSystem& operator=(const RayTracingRenderSystem&) = delete;

		RayTracingRenderSystem(RayTracingRenderSystem&&) = delete;            // Move Constructor
		RayTracingRenderSystem& operator=(RayTracingRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo);
		void createResources();
		void createAccelerationStructures(std::shared_ptr<Scene>& scene);
		void assignTextures(Scene& scene);
		RenderInfo prepareRenderInfo();
		void onResize();

		VkDescriptorSet getCurrentDescriptorSet(int frameIndex) {
			return textureDescriptorSets[frameIndex];
		}

		std::shared_ptr<Framebuffer> getResources() {
			return resources;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout(std::vector<std::unique_ptr<DescriptorSetLayout>>& globalDescriptorSetLayout);

		void createBLAS(std::shared_ptr<Scene>& scene);
		BLASInput objectToGeometry(MeshComponent& model);
		void cmdCreateBLAS(VkCommandBuffer cmdBuffer, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAS, VkDeviceAddress scratchAddress, VkQueryPool queryPool);
		void cmdCompactBLAS(VkCommandBuffer cmdBuffer, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAS, VkQueryPool queryPool);

		void createTLAS(std::shared_ptr<Scene>& scene);
		void buildTLAS(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, bool update = false);

		Device& device;
		DeviceProcedures& deviceProcedures;
		Renderer& renderer;
		std::shared_ptr<Framebuffer> resources;
		std::weak_ptr<Framebuffer> resourcesDepthPrePass;
		Pipeline pipeline{device};

		// Host data to take specialization constants from
		struct SpecializationData {
			// Sets the lighting model used in the fragment "uber" shader
			int numLights;
		} specializationData;

		struct StorageImage {
			VkDeviceMemory memory;
			VkImage image = VK_NULL_HANDLE;
			VkImageView view;
			VkFormat format;
			uint32_t width;
			uint32_t height;
		} storage_image;

		// Acceleration Structures
		std::vector<AccelerationStructure> m_BLAS;
		AccelerationStructure m_TLAS;

		// Textures
		std::unique_ptr<DescriptorSetLayout> textureDescriptorSetLayout{};
		std::vector<VkDescriptorSet> textureDescriptorSets;

		std::unique_ptr<Buffer> uboBuffer;
	};
} // namespace Aspen