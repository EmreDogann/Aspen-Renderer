#include "Aspen/Core/window.hpp"

namespace Aspen {
	static void GLFWErrorCallback(int error, const char* description) {
		throw std::runtime_error(std::string("GLFW Error (") + std::to_string(error) + std::string("): ") + description);
	}

	Window::Window(int width, int height, std::string name)
	    : windowProps(width, height, std::move(name)) {
		initWindow();
	}

	Window::~Window() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Window::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Prevent GLFW from creating an OpenGL context.
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);    // Prevent GLFW from making the window resizeable.

		window = glfwCreateWindow(windowProps.width, windowProps.height, windowProps.windowName.c_str(), nullptr, nullptr);

		glfwSetWindowUserPointer(window, &windowProps);

		// Set GLFW Callbacks
		// Register callback function for GLFW to call whenever the window is resized.
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
			WindowProps& windowProps = *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));
			windowProps.framebufferResized = true;
			windowProps.width = width;
			windowProps.height = height;

			WindowResizeEvent event(width, height);
			windowProps.eventCallback(event);
		});

		glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
			WindowProps& windowProps = *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));

			WindowCloseEvent event;
			windowProps.eventCallback(event);
		});

		glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowProps& windowProps = *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));

			switch (action) {
				case GLFW_PRESS: {
					KeyPressedEvent event(key, 0);
					windowProps.eventCallback(event);
					break;
				}
				case GLFW_RELEASE: {
					KeyReleasedEvent event(key);
					windowProps.eventCallback(event);
					break;
				}
				case GLFW_REPEAT: {
					KeyPressedEvent event(key, 1);
					windowProps.eventCallback(event);
					break;
				}
				default: {
					break;
				}
			}
		});

		glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
			WindowProps& windowProps = *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));
			switch (action) {
				case GLFW_PRESS: {
					MouseButtonPressedEvent event(button);
					windowProps.eventCallback(event);
					break;
				}
				case GLFW_RELEASE: {
					MouseButtonReleasedEvent event(button);
					windowProps.eventCallback(event);
					break;
				}
				default: {
					break;
				}
			}
		});

		glfwSetScrollCallback(window, [](GLFWwindow* window, double xOffset, double yOffset) {
			WindowProps& windowProps = *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));

			MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			windowProps.eventCallback(event);

			Input::UpdateMouseScroll(xOffset, yOffset);
		});

		glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xOffset, double yOffset) {
			WindowProps& windowProps = *static_cast<WindowProps*>(glfwGetWindowUserPointer(window));

			// MouseMovedEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			// windowProps.eventCallback(event);
		});
	}

	void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}
} // namespace Aspen