#include "Aspen/System/camera_controller_system.hpp"

namespace Aspen {
	// void CameraControllerSystem::moveInPlaneXZ(GLFWwindow* window, float dt) {
	// 	glm::vec3 rotate{0.0f};
	// 	// if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) {
	// 	// 	rotate.y += 1.0f;
	// 	// }
	// 	// if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) {
	// 	// 	rotate.y -= 1.0f;
	// 	// }

	// 	// if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) {
	// 	// 	rotate.x += 1.0f;
	// 	// }
	// 	// if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) {
	// 	// 	rotate.x -= 1.0f;
	// 	// }

	// 	const glm::vec3 forwardDir{glm::rotate(transform.rotation, glm::vec3(0.0f, 0.0f, 1.0f))};
	// 	const glm::vec3 rightDir{glm::rotate(transform.rotation, glm::vec3(1.0f, 0.0f, 0.0f))};
	// 	const glm::vec3 upDir{glm::rotate(transform.rotation, glm::vec3(0.0f, 1.0f, 0.0f))};

	// 	// Check that rotate is non-zero.
	// 	// Epsilon is the smallest possible value of a float (but not zero). By comparing, to epsilon instead of 0, we can account for minor rounding errors with floating point numbers.
	// 	if (glm::dot(rotate, rotate) > glm::epsilon<float>()) {
	// 		// The rotate object is normalized so that the object doesn't rotate faster diagonally than in just the horizontal or vertical direction.
	// 		rotate = orbitSpeed * dt * glm::normalize(rotate);
	// 		glm::quat pitch = glm::angleAxis(glm::radians(rotate.x), rightDir);
	// 		glm::quat yaw = glm::angleAxis(glm::radians(rotate.y), glm::vec3(0.0f, 1.0f, 0.0f));
	// 		transform.rotation = glm::normalize(pitch * yaw * transform.rotation);
	// 		// transform.rotation += glm::quat(lookSpeed * dt * glm::normalize(rotate));
	// 	}

	// 	// transform.rotation.x = glm::clamp(transform.rotation.x, -1.5f, 1.5f);        // Limit pitch values between roughly +/- 85ish degrees.
	// 	// transform.rotation.y = glm::mod(transform.rotation.y, glm::two_pi<float>()); // Prevent yaw from going over 360 degrees.

	// 	// Uses old Euler rotations
	// 	// float pitch = transform.rotation.x;
	// 	// float yaw = transform.rotation.y;
	// 	// const glm::vec3 forwardDir{sin(yaw), -sin(pitch), cos(yaw)};
	// 	// const glm::vec3 rightDir{forwardDir.z, 0.0f, -forwardDir.x};
	// 	// const glm::vec3 upDir = glm::cross(forwardDir, rightDir);

	// 	glm::vec3 moveDir{0};

	// 	// if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) {
	// 	// 	moveDir += forwardDir;
	// 	// }
	// 	// if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) {
	// 	// 	moveDir -= forwardDir;
	// 	// }

	// 	// if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) {
	// 	// 	moveDir += rightDir;
	// 	// }
	// 	// if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) {
	// 	// 	moveDir -= rightDir;
	// 	// }

	// 	// if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) {
	// 	// 	moveDir += upDir;
	// 	// }
	// 	// if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) {
	// 	// 	moveDir -= upDir;
	// 	// }

	// 	if (glm::dot(moveDir, moveDir) > glm::epsilon<float>()) {
	// 		// The rotate object is normalized so that the object doesn't rotate faster diagonally than in just the horizontal or vertical direction.
	// 		transform.translation += moveSpeed * dt * glm::normalize(moveDir);
	// 	}
	// }

	void CameraControllerSystem::OnUpdate(CameraControllerArcball& controller, glm::vec2 viewportSize) {
		if (Input::IsKeyPressed(Key::LeftAlt)) {
			const glm::vec2& mouse{Input::GetMouseX(), Input::GetMouseY()};
			glm::vec2 deltaAngle = glm::vec2{2 * glm::pi<float>() / viewportSize.x, glm::pi<float>() / viewportSize.y};
			glm::vec2 delta = (mouse - controller.lastMousePosition) * controller.mouseSensitivity;
			controller.lastMousePosition = mouse;

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle)) {
				mousePan(controller, delta * deltaAngle);
			} else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft)) {
				mouseOrbit(controller, delta * deltaAngle);
			} else if (Input::IsMouseButtonPressed(Mouse::ButtonRight)) {
				mouseZoom(controller, delta.y);
			} else if (Input::IsMouseScrolled()) {
				mouseZoom(controller, Input::GetMouseScrollY());
			}
		}
	}

	void CameraControllerSystem::mousePan(CameraControllerArcball& controller, const glm::vec2& mouseDelta) {
		controller.positionDisplacement.x -= mouseDelta.x;
		controller.positionDisplacement.y -= mouseDelta.y;
		// std::cout << "Displacement: (x) " << controller.positionDisplacement.x << ", (y) " << controller.positionDisplacement.y << std::endl;
	}

	void CameraControllerSystem::mouseOrbit(CameraControllerArcball& controller, const glm::vec2& mouseDelta) {
		glm::vec2 rotate = controller.orbitSpeed * mouseDelta;
		controller.pitch += glm::radians(rotate.y);
		controller.yaw += glm::radians(rotate.x);

		// controller.pitch = glm::clamp(controller.pitch, -glm::pi<float>() / 2, glm::pi<float>() / 2); // Limit to [-90,90] degrees.
		controller.pitch = glm::mod(controller.pitch, glm::two_pi<float>()); // Prevent from going over 360 degrees.
		controller.yaw = glm::mod(controller.yaw, glm::two_pi<float>());     // Prevent from going over 360 degrees.

		// std::cout << "Pitch: " << controller.pitch << ", Yaw: " << controller.yaw << std::endl;
	}

	void CameraControllerSystem::mouseLook(CameraControllerArcball& controller, const glm::vec2& mouseDelta) {
		// const glm::vec3 rightDir{glm::rotate(rotation, glm::vec3(1.0f, 0.0f, 0.0f))};

		// glm::vec2 rotate = controller.orbitSpeed * mouseDelta;
		// glm::quat pitch = glm::angleAxis(glm::radians(rotate.y), rightDir);
		// glm::quat yaw = glm::angleAxis(glm::radians(-rotate.x), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void CameraControllerSystem::mouseZoom(CameraControllerArcball& controller, const float& mouseDelta) {
		controller.distance -= mouseDelta * controller.zoomSpeed;
		if (controller.distance < 0.0f) {
			controller.distance = 0.0f;
		}

		// std::cout << "Distance: " << controller.distance << std::endl;
	}
} // namespace Aspen