#include "Aspen/System/camera_system.hpp"

namespace Aspen {
	void CameraSystem::OnUpdateArcball(TransformComponent& transform, CameraControllerArcball& controller, float timeStep) {
		// TODO: Fix camera glitching when approaching looking directly up or down.
		glm::quat pitch = glm::angleAxis(controller.pitch, glm::rotate(transform.rotation, glm::vec3(1.0f, 0.0f, 0.0f)));
		glm::quat yaw = glm::angleAxis(controller.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		transform.rotation = glm::normalize(pitch * yaw);

		const glm::vec3 forwardDir{glm::rotate(transform.rotation, glm::vec3(0.0f, 0.0f, 1.0f))};
		const glm::vec3 rightDir{glm::rotate(transform.rotation, glm::vec3(1.0f, 0.0f, 0.0f))};
		const glm::vec3 upDir{glm::rotate(transform.rotation, glm::vec3(0.0f, 1.0f, 0.0f))};

		// Calculate final focal point based displacement caused from panning.
		glm::vec3 finalFocalPoint = controller.focalPoint;
		finalFocalPoint += rightDir * controller.positionDisplacement.x;
		finalFocalPoint += upDir * controller.positionDisplacement.y;

		// Calculate the final camera position -> Focal point - some distance in the negative forward direction.
		transform.translation = finalFocalPoint - forwardDir * controller.distance;
	}
} // namespace Aspen