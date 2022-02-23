#pragma once

// Libs & defines
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

// std
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Aspen {

	class AspenWindow {
	public:
		AspenWindow(int w, int h, std::string name);
		~AspenWindow();

		// Remove copy constructor and operator to prevent accidental copy creation of window, possibly leading to a dangling pointer.
		AspenWindow(const AspenWindow &) = delete;
		AspenWindow &operator=(const AspenWindow &);

		AspenWindow(const AspenWindow &&) = delete;
		AspenWindow &operator=(const AspenWindow &&) = delete;

		bool shouldClose() {
			return glfwWindowShouldClose(window);
		}
		VkExtent2D getExtent() const {
			return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
		}

		GLFWwindow *getGLFWwindow() const {
			return window;
		}

		void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

		bool wasWindowResized() const {
			return framebufferResized;
		};
		void resetWindowResizedFlag() {
			framebufferResized = false;
		};

		void windowResizeSubscribe(const std::function<void()> &subscriber) {
			windowResizeSubscribers.push_back(subscriber);
		};
		void notifySubscribers();

	private:
		static void framebufferResizedCallback(GLFWwindow *window, int width, int height);
		void initWindow();

		int width;
		int height;
		bool framebufferResized = false;

		std::string windowName;
		GLFWwindow *window{};

		std::vector<std::function<void()>> windowResizeSubscribers;
	};

} // namespace Aspen