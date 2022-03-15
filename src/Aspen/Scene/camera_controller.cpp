// #include "Aspen/Scene/camera_controller.hpp"
// #include "Aspen/Core/input.hpp"
// #include <iostream>

// namespace Aspen {
// 	void CameraController::OnUpdate(TransformComponent& transform, float timeStep, glm::vec2 viewportSize) {
// 		if (Input::IsKeyPressed(Key::LeftAlt)) {
// 			const glm::vec2& mouse{Input::GetMouseX(), Input::GetMouseY()};
// 			glm::vec2 deltaAngle = glm::vec2{2 * glm::pi<float>() / viewportSize.x, glm::pi<float>() / viewportSize.y};
// 			glm::vec2 delta = (mouse - lastMousePosition) * mouseSensitivity;
// 			lastMousePosition = mouse;

// 			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle)) {
// 				mousePan(delta * deltaAngle);
// 			} else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft)) {
// 				mouseOrbit(delta * deltaAngle);
// 			} else if (Input::IsMouseButtonPressed(Mouse::ButtonRight)) {
// 				mouseZoom(delta.y);
// 			}
// 		}
// 		// updateTransform();
// 		glm::vec3 offset = glm::rotate(transform.rotation, glm::vec3(0.0f, 0.0f, 1.0f)) * distance;
// 		transform.translation = focalPoint - offset;
// 	}

// 	void CameraController::OnEvent(Event& e) {
// 		EventDispatcher dispatcher(e);
// 		dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(CameraController::OnMouseScroll));
// 	}

// 	bool CameraController::OnMouseScroll(MouseScrolledEvent& e) {
// 		float delta = e.GetYOffset() * 0.1f;
// 		mouseZoom(delta);
// 		updateTransform();
// 		return false;
// 	}

// 	void CameraController::mousePan(const glm::vec2& mouseDelta) {
// 		focalPoint += glm::rotate(cameraTransform->rotation, glm::vec3(1.0f, 0.0f, 0.0f)) * mouseDelta.x * distance;
// 		focalPoint += glm::rotate(cameraTransform->rotation, glm::vec3(0.0f, 1.0f, 0.0f)) * mouseDelta.y * distance;
// 		std::cout << focalPoint.x << ", " << focalPoint.y << ", " << focalPoint.z << std::endl;
// 	}

// 	void CameraController::mouseOrbit(const glm::vec2& mouseDelta) {
// 		const glm::vec3 rightDir{glm::rotate(cameraTransform->rotation, glm::vec3(1.0f, 0.0f, 0.0f))};

// 		glm::vec2 rotate = orbitSpeed * mouseDelta;
// 		glm::quat pitch = glm::angleAxis(glm::radians(rotate.y), rightDir);
// 		glm::quat yaw = glm::angleAxis(glm::radians(-rotate.x), glm::vec3(0.0f, 1.0f, 0.0f));
// 		cameraTransform->rotation = glm::normalize(pitch * yaw * cameraTransform->rotation);
// 	}

// 	void CameraController::mouseLook(const glm::vec2& mouseDelta) {
// 		const glm::vec3 rightDir{glm::rotate(cameraTransform->rotation, glm::vec3(1.0f, 0.0f, 0.0f))};

// 		// The rotate object is normalized so that the object doesn't rotate faster diagonally than in just the horizontal or vertical direction.
// 		glm::vec2 rotate = orbitSpeed * glm::normalize(mouseDelta);
// 		glm::quat pitch = glm::angleAxis(glm::radians(rotate.y), rightDir);
// 		glm::quat yaw = glm::angleAxis(glm::radians(rotate.x), glm::vec3(0.0f, 1.0f, 0.0f));
// 		cameraTransform->rotation = glm::normalize(pitch * yaw * cameraTransform->rotation);
// 		// transform.rotation += glm::quat(lookSpeed * dt * glm::normalize(rotate));
// 	}
// 	void CameraController::mouseZoom(const float mouseDelta) {
// 		const glm::vec3 forwardDir{glm::rotate(cameraTransform->rotation, glm::vec3(0.0f, 0.0f, 1.0f))};

// 		distance -= mouseDelta * zoomSpeed;
// 		if (distance < 1.0f) {
// 			// focalPoint += forwardDir;
// 			distance = 1.0f;
// 		}

// 		std::cout << "Distance: " << distance << std::endl;
// 	}
// } // namespace Aspen