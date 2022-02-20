#include "aspen_window.hpp"

namespace Aspen {

	AspenWindow::AspenWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{std::move(name)} {
		initWindow();
	}

	AspenWindow::~AspenWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void AspenWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Prevent GLFW from creating an OpenGL context.
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);    // Prevent GLFW from making the window resizeable.

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);

		// Register callback function for GLFW to call whenever the window is resized.
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
	}

	void AspenWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	void AspenWindow::framebufferResizedCallback(GLFWwindow *window, int width, int height) {
		auto *aspenWindow = reinterpret_cast<AspenWindow *>(glfwGetWindowUserPointer(window));
		aspenWindow->framebufferResized = true;
		aspenWindow->width = width;
		aspenWindow->height = height;

		aspenWindow->notifySubscribers(); // Notify subscribers when window has been resized.
	}

	// Call all subscriber functions on an event occuring (e.g. window resize).
	void AspenWindow::notifySubscribers() {
		for (const auto &function : windowResizeSubscribers) {
			function();
		}
	}
} // namespace Aspen