#pragma once
#include "pch.h"

#include "Aspen/Core/buffer.hpp"
#include "Aspen/Core/timer.hpp"
#include "Aspen/Core/window.hpp"
#include "Aspen/Math/math.hpp"
#include "Aspen/Renderer/camera.hpp"
#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/renderer.hpp"
#include "Aspen/Renderer/simple_render_system.hpp"
#include "Aspen/Scene/components.hpp"
#include "Aspen/Scene/entity.hpp"
#include "Aspen/Scene/scene.hpp"
#include "Aspen/System/camera_controller_system.hpp"
#include "Aspen/System/camera_system.hpp"

// Libs & defines
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#include "ImGuizmo.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui_internal.h>

namespace Aspen {
	class Application {
	public:
		static constexpr int WIDTH = 1280;
		static constexpr int HEIGHT = 720;
		static constexpr float MAX_FRAME_TIME = 0.1f;

		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		Application(const Application&&) = delete;
		Application& operator=(const Application&&) = delete;

		void run();
		void OnEvent(Event& e);

		static Application& getInstance() {
			return *s_Instance;
		}

		Window& getWindow() {
			return window;
		}

	private:
		void loadGameObjects();
		void renderUI(VkCommandBuffer commandBuffer, Camera camera);
		void setupImGui();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		static Application* s_Instance;

		Window window{WIDTH, HEIGHT, "Hello Vulkan!"};
		Device device{window};
		Buffer bufferManager{};
		Renderer renderer{window, device, bufferManager};
		SimpleRenderSystem simpleRenderSystem{device, bufferManager, renderer};
		Timer timer{};

		Entity cameraEntity{}; // Empty entity to store the transformation of the camera.
		Entity object{};
		Entity floor{};

		bool m_Running = true;

		std::shared_ptr<Scene> m_Scene;
		float currentFrameTime = 0.0f;

		int gizmoType = -1;
		bool snapping = false;
		VkDescriptorSet viewportTexture{};

		glm::vec2 viewportSize{WIDTH, HEIGHT};
		glm::vec2 viewportBounds[2]{};
	};
} // namespace Aspen