#pragma once

#include <pch.h>

// Libs & defines
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "Aspen/Core/input.hpp"
#include "Aspen/Events/application_event.hpp"
#include "Aspen/Events/key_event.hpp"
#include "Aspen/Events/mouse_event.hpp"

namespace Aspen {

	class Window {
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		Window(int width, int height, std::string name);
		~Window();

		// Remove copy constructor and operator to prevent accidental copy creation of window, possibly leading to a dangling pointer.
		Window(const Window&) = delete;
		Window& operator=(const Window&);

		Window(const Window&&) = delete;
		Window& operator=(const Window&&) = delete;

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

		// Process window level events (such as keystrokes).
		void OnUpdate() {
			glfwPollEvents();
		}

		void setEventCallback(const EventCallbackFn& callback) {
			windowProps.eventCallback = callback;
		}

		void setWindowTitle(const std::string& title) {
			glfwSetWindowTitle(window, title.c_str());
		}

		void setExtent(int width, int height) {
			windowProps.width = width;
			windowProps.height = height;
		}

		VkExtent2D getExtent() const {
			return {static_cast<uint32_t>(windowProps.width), static_cast<uint32_t>(windowProps.height)};
		}

		GLFWwindow* getGLFWwindow() const {
			return window;
		}

		std::string getWindowTitle() const {
			return windowProps.windowName;
		}

		bool wasWindowResized() const {
			return windowProps.framebufferResized;
		};
		void resetWindowResizedFlag() {
			windowProps.framebufferResized = false;
		};

	private:
		void initWindow();

		GLFWwindow* window{};

		struct WindowProps {
			WindowProps(int width, int height, std::string name)
			    : width{width}, height{height}, windowName{std::move(name)} {}
			std::string windowName;
			int width, height;
			bool framebufferResized = false;

			EventCallbackFn eventCallback;
		} windowProps;
	};

} // namespace Aspen