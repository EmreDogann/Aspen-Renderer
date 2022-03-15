#include "Aspen/System/camera_system.hpp"
#include <iostream>

namespace Aspen {
	void CameraSystem::OnUpdateArcball(TransformComponent& transform, CameraControllerArcball& controller, float timeStep) {
		float invertedPitch = -controller.pitch; // Flip the pitch direction to get inverted pitching.

		// Compute the forward and right vectors based on the yaw and pitch.
		glm::vec3 forwardDir = glm::normalize(glm::vec3(sin(controller.yaw), -sin(invertedPitch), cos(controller.yaw)));
		glm::vec3 rightDir = glm::normalize(glm::vec3(forwardDir.z, 0.0f, -forwardDir.x));

		// Compute the rotation quaternions to apply.
		glm::quat qPitch = glm::angleAxis(invertedPitch, rightDir);                   // Local right axis.
		glm::quat qYaw = glm::angleAxis(controller.yaw, glm::vec3(0.0f, 1.0f, 0.0f)); // Global up axis to prevent rolling.
		transform.rotation = glm::normalize(qPitch * qYaw);

		// Recalculate the orthogonal base vectors using the new rotation quaternion.
		forwardDir = {glm::rotate(transform.rotation, glm::vec3(0.0f, 0.0f, 1.0f))};
		rightDir = {glm::rotate(transform.rotation, glm::vec3(1.0f, 0.0f, 0.0f))};
		glm::vec3 upDir = {glm::rotate(transform.rotation, glm::vec3(0.0f, 1.0f, 0.0f))};

		// Update the focal point based on the displacement produced from panning.
		controller.focalPoint += rightDir * controller.positionDisplacement.x;
		controller.focalPoint += upDir * controller.positionDisplacement.y;
		controller.positionDisplacement = glm::vec2{0.0f}; // Reset the pan value.

		// Calculate the final camera position -> Focal point - some distance in the negative forward direction.
		transform.translation = controller.focalPoint - forwardDir * controller.distance;
	}
} // namespace Aspen