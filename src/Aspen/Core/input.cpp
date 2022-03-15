#include "Aspen/Core/input.hpp"
#include "Aspen/Core/application.hpp"

namespace Aspen {

	bool Input::IsKeyPressed(const KeyCode key) {
		auto* window = static_cast<GLFWwindow*>(Application::getInstance().getWindow().getGLFWwindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(const MouseCode button) {
		auto* window = static_cast<GLFWwindow*>(Application::getInstance().getWindow().getGLFWwindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseScrolled() {
		return scrollValue.x != 0.0f || scrollValue.y != 0.0f;
	}

	glm::vec2 Input::GetMousePosition() {
		auto* window = static_cast<GLFWwindow*>(Application::getInstance().getWindow().getGLFWwindow());
		double xPos = NAN;
		double yPos = NAN;
		glfwGetCursorPos(window, &xPos, &yPos);

		return {static_cast<float>(xPos), static_cast<float>(yPos)};
	}

	float Input::GetMouseX() {
		return GetMousePosition().x;
	}

	float Input::GetMouseY() {
		return GetMousePosition().y;
	}

	void Input::UpdateMouseScroll(float x, float y) {
		scrollValue.x = x;
		scrollValue.y = y;
	}

	float Input::GetMouseScrollX() {
		return scrollValue.x;
	}

	float Input::GetMouseScrollY() {
		return scrollValue.y;
	}

	// This will be called every frame in the game loop.
	void Input::OnUpdate() {
		// Reset the scroll value.
		scrollValue = glm::vec2{};
	}

} // namespace Aspen