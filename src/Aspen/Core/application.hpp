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
#include "Aspen/Scene/camera_controller.hpp"
#include "Aspen/Scene/components.hpp"
#include "Aspen/Scene/entity.hpp"
#include "Aspen/Scene/scene.hpp"

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

namespace Aspen {
	class Application {
	public:
		static constexpr int WIDTH = 1024;
		static constexpr int HEIGHT = 768;
		static constexpr float MAX_FRAME_TIME = 0.1f;

		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		Application(const Application&&) = delete;
		Application& operator=(const Application&&) = delete;

		void run();
		void OnEvent(Event& e);

	private:
		void loadGameObjects();
		void renderUI(VkCommandBuffer commandBuffer, AspenCamera camera);
		void setupImGui();
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		AspenWindow aspenWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
		AspenDevice aspenDevice{aspenWindow};
		Buffer bufferManager{aspenDevice};
		AspenRenderer aspenRenderer{aspenWindow, aspenDevice, bufferManager};
		SimpleRenderSystem simpleRenderSystem{aspenDevice, bufferManager, aspenRenderer};
		Timer timer{};

		Entity cameraEntity{};               // Empty entity to store the transformation of the camera.
		CameraController cameraController{}; // Input handler for the camera.

		Entity cube{};

		bool m_Running = true;

		std::shared_ptr<Scene> m_Scene;

		int gizmoType = -1;
		bool snapping = false;
		VkDescriptorSet viewportTexture{};
	};
} // namespace Aspen