#include "device_procedures.hpp"
#include "device.hpp"

// From: https://github.com/GPSnoopy/RayTracingInVulkan/blob/master/src/Vulkan/RayTracing/DeviceProcedures.cpp
namespace Aspen {

	template <class Func>
	Func GetProcedure(const Device& device, const char* const name) {
		const auto func = reinterpret_cast<Func>(vkGetDeviceProcAddr(device.device(), name));
		if (func == nullptr) {
			throw std::runtime_error(std::string("failed to get address of '") + name + "'");
		}

		return func;
	}

	DeviceProcedures::DeviceProcedures(const class Device& device)
	    : vkCreateAccelerationStructureKHR(GetProcedure<PFN_vkCreateAccelerationStructureKHR>(device, "vkCreateAccelerationStructureKHR")),
	      vkDestroyAccelerationStructureKHR(GetProcedure<PFN_vkDestroyAccelerationStructureKHR>(device, "vkDestroyAccelerationStructureKHR")),
	      vkGetAccelerationStructureBuildSizesKHR(GetProcedure<PFN_vkGetAccelerationStructureBuildSizesKHR>(device, "vkGetAccelerationStructureBuildSizesKHR")),
	      vkCmdBuildAccelerationStructuresKHR(GetProcedure<PFN_vkCmdBuildAccelerationStructuresKHR>(device, "vkCmdBuildAccelerationStructuresKHR")),
	      vkCmdCopyAccelerationStructureKHR(GetProcedure<PFN_vkCmdCopyAccelerationStructureKHR>(device, "vkCmdCopyAccelerationStructureKHR")),
	      vkCmdTraceRaysKHR(GetProcedure<PFN_vkCmdTraceRaysKHR>(device, "vkCmdTraceRaysKHR")),
	      vkCreateRayTracingPipelinesKHR(GetProcedure<PFN_vkCreateRayTracingPipelinesKHR>(device, "vkCreateRayTracingPipelinesKHR")),
	      vkGetRayTracingShaderGroupHandlesKHR(GetProcedure<PFN_vkGetRayTracingShaderGroupHandlesKHR>(device, "vkGetRayTracingShaderGroupHandlesKHR")),
	      vkGetAccelerationStructureDeviceAddressKHR(GetProcedure<PFN_vkGetAccelerationStructureDeviceAddressKHR>(device, "vkGetAccelerationStructureDeviceAddressKHR")),
	      vkGetBufferDeviceAddressKHR(GetProcedure<PFN_vkGetBufferDeviceAddressKHR>(device, "vkGetBufferDeviceAddressKHR")),
	      vkCmdWriteAccelerationStructuresPropertiesKHR(GetProcedure<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(device, "vkCmdWriteAccelerationStructuresPropertiesKHR")),
	      device_(device) {
	}

	DeviceProcedures::~DeviceProcedures() {
	}

} // namespace Aspen
