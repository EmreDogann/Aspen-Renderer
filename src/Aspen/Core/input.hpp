#pragma once
#include "Aspen/Core/keycodes.hpp"
#include "Aspen/Core/mousecodes.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Aspen {
	class Input {
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(MouseCode button);
		static bool IsMouseScrolled();
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
		static void UpdateMouseScroll(float x, float y);
		static float GetMouseScrollX();
		static float GetMouseScrollY();
		static void OnUpdate();

	private:
		inline static glm::vec2 scrollValue{};
	};
} // namespace Aspen