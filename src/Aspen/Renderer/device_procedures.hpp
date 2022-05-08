#pragma once
#include "pch.h"
#include "vulkan/vulkan.h"

// From:: https://github.com/GPSnoopy/RayTracingInVulkan/blob/master/src/Vulkan/RayTracing/DeviceProcedures.hpp
namespace Aspen {
	class Device;

	class DeviceProcedures final {
	public:
		DeviceProcedures(const DeviceProcedures&) = delete;

		explicit DeviceProcedures(const Device& device);
		~DeviceProcedures();

		const std::function<VkResult(
		    VkDevice device,
		    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
		    const VkAllocationCallbacks* pAllocator,
		    VkAccelerationStructureKHR* pAccelerationStructure)>
		    vkCreateAccelerationStructureKHR;

		const std::function<void(
		    VkDevice device,
		    VkAccelerationStructureKHR accelerationStructure,
		    const VkAllocationCallbacks* pAllocator)>
		    vkDestroyAccelerationStructureKHR;

		const std::function<void(
		    VkDevice device,
		    VkAccelerationStructureBuildTypeKHR buildType,
		    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
		    const uint32_t* pMaxPrimitiveCounts,
		    VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)>
		    vkGetAccelerationStructureBuildSizesKHR;

		const std::function<void(
		    VkCommandBuffer commandBuffer,
		    uint32_t infoCount,
		    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
		    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)>
		    vkCmdBuildAccelerationStructuresKHR;

		const std::function<void(
		    VkCommandBuffer commandBuffer,
		    const VkCopyAccelerationStructureInfoKHR* pInfo)>
		    vkCmdCopyAccelerationStructureKHR;

		const std::function<void(
		    VkCommandBuffer commandBuffer,
		    const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
		    const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
		    const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
		    const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
		    uint32_t width,
		    uint32_t height,
		    uint32_t depth)>
		    vkCmdTraceRaysKHR;

		const std::function<VkResult(
		    VkDevice device,
		    VkDeferredOperationKHR deferredOperation,
		    VkPipelineCache pipelineCache,
		    uint32_t createInfoCount,
		    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
		    const VkAllocationCallbacks* pAllocator,
		    VkPipeline* pPipelines)>
		    vkCreateRayTracingPipelinesKHR;

		const std::function<VkResult(
		    VkDevice device,
		    VkPipeline pipeline,
		    uint32_t firstGroup,
		    uint32_t groupCount,
		    size_t dataSize,
		    void* pData)>
		    vkGetRayTracingShaderGroupHandlesKHR;

		const std::function<VkDeviceAddress(
		    VkDevice device,
		    const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)>
		    vkGetAccelerationStructureDeviceAddressKHR;

		const std::function<VkDeviceAddress(
		    VkDevice device,
		    const VkBufferDeviceAddressInfo* pInfo)>
		    vkGetBufferDeviceAddressKHR;

		const std::function<void(
		    VkCommandBuffer commandBuffer,
		    uint32_t accelerationStructureCount,
		    const VkAccelerationStructureKHR* pAccelerationStructures,
		    VkQueryType queryType,
		    VkQueryPool queryPool,
		    uint32_t firstQuery)>
		    vkCmdWriteAccelerationStructuresPropertiesKHR;

	private:
		const class Device& device_;
	};
} // namespace Aspen
