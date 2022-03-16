#pragma once

#include "Aspen/Scene/components.hpp"

namespace Aspen {
	class CameraSystem {
	public:
		static void OnUpdateArcball(TransformComponent& transform, CameraControllerArcball& controller, float timeStep);
		static void setTarget(TransformComponent& transform, CameraControllerArcball& controller, glm::vec3 target);
	};
} // namespace Aspen