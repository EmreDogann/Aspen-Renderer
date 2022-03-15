#pragma once

#include "Aspen/Renderer/camera.hpp"

// Libs
#include <vulkan/vulkan.h>

namespace Aspen {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		Camera& camera;
	};
} // namespace Aspen