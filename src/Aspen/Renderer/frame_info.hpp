#pragma once

#include "Aspen/Scene/scene.hpp"

namespace Aspen {
	struct RenderInfo {
		VkFramebuffer framebuffer;
		VkRenderPass renderPass;

		VkRect2D scissorDimensions{};
		VkViewport viewport{};
		std::vector<VkClearValue> clearValues;
	};

	struct ApplicationState {
		ApplicationState() {
			time_averager.resize(TIME_HISTORY_COUNT);
			for (int i = 0; i < TIME_HISTORY_COUNT; ++i) {
				time_averager[i] = FIXED_DELTA_TIME;
			}
		}

		bool stateChanged = false;

		int update_rate = 240; // 240 Fps
		double FIXED_DELTA_TIME = 1.0 / static_cast<double>(update_rate);
		double MAX_FRAME_TIME = 8.0 / 60.0; // 7.5 Fps
		int TIME_HISTORY_COUNT = 4;
		int UPDATE_MULTIPLICITY = 1;

		int useRayTracer = 0;
		bool useTextureMapping = true;

		bool useShadows = true;
		float rasterShadowBias = 0.00001;
		float rtShadowBias = 0.05;
		float rasterShadowOpacity = 0.1;
		float rtShadowOpacity = 0.1;

		bool useRTReflections = true;
		bool useRTRefractions = true;
		int rtTotalBounces = 10;
		float rayMinRange = 0.001;

		bool enable_vsync = true;
		int fps_cap = 60;

		bool resync = true;
		bool vsync_snapping = false;
		float vsync_error = 0.0002f;
		// Circular_Buffer<double> time_averager{TIME_HISTORY_COUNT};
		// double time_averager[4] = {FIXED_DELTA_TIME, FIXED_DELTA_TIME, FIXED_DELTA_TIME, FIXED_DELTA_TIME};
		std::vector<double> time_averager;
		double snap_frequencies[7] = {
		    1.0 / 60.0,              // 60fps
		    (1.0 / 60.0) * 2,        // 30fps
		    (1.0 / 60.0) * 2.5,      // 24fps
		    (1.0 / 60.0) * 3,        // 20fps
		    (1.0 / 60.0) * 4,        // 15fps
		    (1.0 / 60.0 + 1) / 2.0,  // 120fps // 120hz, 240hz, or higher need to round up, so that adding 120hz twice guaranteed is at least the same as adding time_60hz once
		    (1.0 / 60.0 + 1) / 2.75, // 165fps // that's where the +1 and +2 come from in those equations
		    // (1.0f / 60.0f + 2) / 3.0f,  // 180fps // I do not want to snap to anything higher than 120 in my engine, but I left the math in here anyway
		    // (1.0f / 60.0f + 3) / 4.0f,  // 240fps
		};

		int totalVertexCount = 0;
		int totalIndexCount = 0;
	};

	struct FrameInfo {
		int frameIndex;
		float frameTime;
		std::vector<VkDescriptorSet> descriptorSet;
		uint32_t dynamicOffset;
		VkCommandBuffer commandBuffer;
		Camera& camera;
		std::shared_ptr<Scene>& scene;
		ApplicationState appState;
	};
} // namespace Aspen