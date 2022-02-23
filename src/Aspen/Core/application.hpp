#pragma once
#include "Aspen/Core/model.hpp"
#include "Aspen/Core/window.hpp"
#include "Aspen/Renderer/camera.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/renderer.hpp"
#include "Aspen/Renderer/simple_render_system.hpp"
#include "Aspen/Scene/camera_controller.hpp"
#include "Aspen/Scene/game_object.hpp"


// Libs & defines
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