#pragma once

// Adapted from https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Core/Timer.h

#include "pch.h"

#include <GLFW/glfw3.h>

namespace Aspen {
	class Timer {
	public:
		Timer() {
			reset();
		}

		void reset() {
			// m_start = std::chrono::high_resolution_clock::now();
			m_start = glfwGetTime();
		}

		double elapsedSeconds() {
			// return std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - m_start).count();
			return glfwGetTime() - m_start;
		}

		double elapsedMillis() {
			return elapsedSeconds() * 1000.0;
		}

	private:
		// std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
		double m_start = 0.0;
	};
} // namespace Aspen