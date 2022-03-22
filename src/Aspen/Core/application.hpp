#pragma once
#include "pch.h"
#include "Aspen/Utils/circular_buffer.hpp"

#include "Aspen/Core/timer.hpp"
#include "Aspen/Core/window.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/renderer.hpp"
#include "Aspen/Renderer/System/simple_render_system.hpp"
#include "Aspen/Renderer/System/point_light_render_system.hpp"
#include "Aspen/Renderer/System/ui_render_system.hpp"
#include "Aspen/Renderer/System/mouse_picking_render_system.hpp"
#include "Aspen/Renderer/System/depth_prepass_render_system.hpp"
#include "Aspen/Scene/entity.hpp"
#include "Aspen/System/camera_controller_system.hpp"
#include "Aspen/System/camera_system.hpp"

// Libs & defines
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.

// #define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)
#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Aspen {
	struct TimerState {
		static constexpr double UPDATE_RATE = 240.0; // 240 Fps
		static constexpr double FIXED_DELTA_TIME = 1.0 / UPDATE_RATE;
		static constexpr double MAX_FRAME_TIME = 8.0 / 60.0; // 7.5 Fps
		static constexpr double VSYNC_ERROR = 0.0002;
		static constexpr int TIME_HISTORY_COUNT = 4;
		static constexpr int UPDATE_MULTIPLICITY = 1;
		static constexpr bool UNLOCK_FRAMERATE = false;

		bool resync = true;
		// Circular_Buffer<double> time_averager{TIME_HISTORY_COUNT};
		double time_averager[TIME_HISTORY_COUNT] = {FIXED_DELTA_TIME, FIXED_DELTA_TIME, FIXED_DELTA_TIME, FIXED_DELTA_TIME};

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
	};

	class Application {
	public:
		static constexpr int WIDTH = 1280;
		static constexpr int HEIGHT = 720;

		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		Application(const Application&&) = delete;
		Application& operator=(const Application&&) = delete;

		void run();
		void update(double deltaTime);
		void render(double deltaTime);
		void OnEvent(Event& e);

		static Application& getInstance() {
			return *s_Instance;
		}

		Window& getWindow() {
			return window;
		}

	private:
		void loadEntities();
		void renderUI(VkCommandBuffer commandBuffer, Camera camera);
		void setupImGui();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		inline static Application* s_Instance = nullptr;

		Window window{WIDTH, HEIGHT, "Hello Vulkan!"};
		Device device{window};
		Renderer renderer{window, device};

		GlobalRenderSystem globalRenderSystem{device, renderer};
		SimpleRenderSystem simpleRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout()};
		PointLightRenderSystem pointLightRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout()};
		UIRenderSystem uiRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout()};
		MousePickingRenderSystem mousePickingRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout()};
		DepthPrePassRenderSystem depthPrePassRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout()};

		Timer timer{};
		TimerState timerState{};
		float fpsUpdateCooldown = 1.0f;

		Entity cameraEntity{}; // Empty entity to store the transformation of the camera.
		Entity object{};
		Entity floor{};

		bool m_Running = true;

		std::shared_ptr<Scene> m_Scene;

		UIState uiState{};
		bool mousePicking = false;
	};
} // namespace Aspen