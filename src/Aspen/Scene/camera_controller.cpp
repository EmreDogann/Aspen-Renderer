#include "Aspen/Scene/camera_controller.hpp"

namespace Aspen {
	void CameraController::moveInPlaneXZ(GLFWwindow* window, float dt, TransformComponent& transform) {
		glm::vec3 rotate{0};
		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) {
			rotate.y += 1.0f;
		}
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) {
			rotate.y -= 1.0f;
		}

		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) {
			rotate.x += 1.0f;
		}
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) {
			rotate.x -= 1.0f;
		}

		// Check that rotate is non-zero.
		// Epsilon is the smallest possible value of a float (but not zero). By comparing, to epsilon instead of 0, we can account for minor rounding errors with floating point numbers.
		if (glm::dot(rotate, rotate) > glm::epsilon<float>()) {
			// The rotate object is normalized so that the object doesn't rotate faster diagonally than in just the horizontal or vertical direction.
			transform.rotation += lookSpeed * dt * glm::normalize(rotate);
		}

		transform.rotation.x = glm::clamp(transform.rotation.x, -1.5f, 1.5f);        // Limit pitch values between roughly +/- 85ish degrees.
		transform.rotation.y = glm::mod(transform.rotation.y, glm::two_pi<float>()); // Prevent yaw from going over 360 degrees.

		float pitch = transform.rotation.x;
		float yaw = transform.rotation.y;
		const glm::vec3 forwardDir{sin(yaw), -sin(pitch), cos(yaw)};
		const glm::vec3 rightDir{forwardDir.z, 0.0f, -forwardDir.x};
		const glm::vec3 upDir = glm::cross(forwardDir, rightDir);

		glm::vec3 moveDir{0};

		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
			moveDir += forwardDir;
		}
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
			moveDir -= forwardDir;
		}

		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
			moveDir += rightDir;
		}
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
			moveDir -= rightDir;
		}

		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) {
			moveDir += upDir;
		}
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) {
			moveDir -= upDir;
		}

		if (glm::dot(moveDir, moveDir) > glm::epsilon<float>()) {
			// The rotate object is normalized so that the object doesn't rotate faster diagonally than in just the horizontal or vertical direction.
			transform.translation += moveSpeed * dt * glm::normalize(moveDir);
		}
	}
} // namespace Aspen