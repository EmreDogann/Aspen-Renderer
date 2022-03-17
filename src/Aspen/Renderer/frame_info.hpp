#pragma once

#include "Aspen/Renderer/camera.hpp"
#include "Aspen/Scene/scene.hpp"

// Libs
#include <vulkan/vulkan.h>

namespace Aspen {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkDescriptorSet descriptorSet;
		VkCommandBuffer commandBuffer;
		Camera& camera;
		std::shared_ptr<Scene>& scene;
	};
} // namespace Aspen