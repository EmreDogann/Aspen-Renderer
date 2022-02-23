#pragma once
#include "aspen_camera.hpp"
#include "aspen_device.hpp"
#include "aspen_game_object.hpp"
#include "aspen_model.hpp"
#include "aspen_renderer.hpp"
#include "aspen_window.hpp"
#include "camera_controller.hpp"
#include "simple_render_system.hpp"

// Libs & defines
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace Aspen {
	class FirstApp {
	public:
		static constexpr int WIDTH = 1024;
		static constexpr int HEIGHT = 768;
		static constexpr float MAX_FRAME_TIME = 1.0f;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp &) = delete;
		FirstApp &operator=(const FirstApp &) = delete;
		FirstApp(const FirstApp &&) = delete;
		FirstApp &operator=(const FirstApp &&) = delete;

		void run();

	private:
		void loadGameObjects();

		AspenWindow aspenWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
		AspenDevice aspenDevice{aspenWindow};
		AspenRenderer aspenRenderer{aspenWindow, aspenDevice};

		std::vector<AspenGameObject> gameObjects;
	};
} // namespace Aspen