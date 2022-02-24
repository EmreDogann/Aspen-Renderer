#pragma once

#include <pch.h>

// Libs & defines
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "Aspen/Events/application_event.hpp"
#include "Aspen/Events/key_event.hpp"
#include "Aspen/Events/mouse_event.hpp"

namespace Aspen {

	class AspenWindow {
	public:
		using EventCallbackFn = std::function<void(Event &)>;

		AspenWindow(int width, int height, std::string name);
		~AspenWindow();

		// Remove copy constructor and operator to prevent accidental copy creation of window, possibly leading to a dangling pointer.
		AspenWindow(const AspenWindow &) = delete;
		AspenWindow &operator=(const AspenWindow &);

		AspenWindow(const AspenWindow &&) = delete;
		AspenWindow &operator=(const AspenWindow &&) = delete;

		void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

		// Process window level events (such as keystrokes).
		void OnUpdate() {
			glfwPollEvents();
		}

		void setEventCallback(const EventCallbackFn &callback) {
			windowProps.eventCallback = callback;
		}

		VkExtent2D getExtent() const {
			return {static_cast<uint32_t>(windowProps.width), static_cast<uint32_t>(windowProps.height)};
		}

		GLFWwindow *getGLFWwindow() const {
			return window;
		}

		bool wasWindowResized() const {
			return windowProps.framebufferResized;
		};
		void resetWindowResizedFlag() {
			windowProps.framebufferResized = false;
		};

	private:
		void initWindow();

		GLFWwindow *window{};

		struct WindowProps {
			WindowProps(int width, int height, std::string name) : width{width}, height{height}, windowName{std::move(name)} {}
			std::string windowName;
			int width, height;
			bool framebufferResized = false;

			EventCallbackFn eventCallback;
		} windowProps;
	};

} // namespace Aspen