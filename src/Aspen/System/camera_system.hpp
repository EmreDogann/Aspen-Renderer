#pragma once

#include "Aspen/Scene/components.hpp"

namespace Aspen {
	class CameraSystem {
	public:
		static void OnUpdateArcball(TransformComponent& transform, CameraControllerArcball& controller, float timeStep);
	};
} // namespace Aspen