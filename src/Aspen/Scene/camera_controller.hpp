// #pragma once

// #include "Aspen/Core/window.hpp"
// #include "Aspen/Scene/components.hpp"

// #define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// namespace Aspen {
// 	class CameraController {
// 	public:
// 		void moveInPlaneXZ(GLFWwindow* window, float dt, TransformComponent& transform);
// 		void updateTransform();
// 		void OnEvent(Event& e);

// 	private:
// 		bool OnMouseScroll(MouseScrolledEvent& e);
// 		void mousePan(const glm::vec2& mouseDelta);
// 		void mouseLook(const glm::vec2& mouseDelta);
// 		void mouseOrbit(const glm::vec2& mouseDelta);
// 		void mouseZoom(const float mouseDelta);
// 	};
// } // namespace Aspen