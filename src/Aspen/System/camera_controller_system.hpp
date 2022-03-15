#pragma once

#include "Aspen/Core/input.hpp"
#include "Aspen/Scene/components.hpp"

namespace Aspen {
	class CameraControllerSystem {
	public:
		static void OnUpdate(CameraControllerArcball& controller, glm::vec2 viewportSize);

	private:
		static void mousePan(CameraControllerArcball& controller, const glm::vec2& mouseDelta);
		static void mouseLook(CameraControllerArcball& controller, const glm::vec2& mouseDelta);
		static void mouseOrbit(CameraControllerArcball& controller, const glm::vec2& mouseDelta);
		static void mouseZoom(CameraControllerArcball& controller, const float& mouseDelta);
	};
} // namespace Aspen