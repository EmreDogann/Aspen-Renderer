#pragma once
#include "pch.h"

#include "Aspen/Core/timer.hpp"
#include "Aspen/Renderer/System/simple_render_system.hpp"
#include "Aspen/Renderer/System/point_light_render_system.hpp"
#include "Aspen/Renderer/System/ui_render_system.hpp"
#include "Aspen/Renderer/System/mouse_picking_render_system.hpp"
#include "Aspen/Renderer/System/depth_prepass_render_system.hpp"
#include "Aspen/Renderer/System/outline_render_system.hpp"
#include "Aspen/Scene/entity.hpp"
#include "Aspen/System/camera_controller_system.hpp"
#include "Aspen/System/camera_system.hpp"

// #define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)
#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Aspen {
	class Application {
	public:
		static constexpr int WIDTH = 1920;
		static constexpr int HEIGHT = 1080;

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

		ApplicationState appState{};
		UIState uiState{};

		Window window{WIDTH, HEIGHT, "Hello Vulkan!"};
		Device device{window};
		Renderer renderer{window, device, !appState.enable_vsync};

		Timer timer{};
		float fpsUpdateCooldown = 1.0f;

		Entity cameraEntity{}; // Empty entity to store the transformation of the camera.
		Entity object{};
		Entity floor{};

		bool m_Running = true;

		std::shared_ptr<Scene> m_Scene;

		bool mousePicking = false;

		GlobalRenderSystem globalRenderSystem{device, renderer};
		DepthPrePassRenderSystem depthPrePassRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout()};
		SimpleRenderSystem simpleRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout(),
		    depthPrePassRenderSystem.getResources()};
		PointLightRenderSystem pointLightRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout(),
		    simpleRenderSystem.getResources()};
		OutlineRenderSystem outlineRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout(),
		    simpleRenderSystem.getResources()};
		UIRenderSystem uiRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout()};
		MousePickingRenderSystem mousePickingRenderSystem{
		    device,
		    renderer,
		    globalRenderSystem.getDescriptorSetLayout(),
		    depthPrePassRenderSystem.getResources()};
	};
} // namespace Aspen