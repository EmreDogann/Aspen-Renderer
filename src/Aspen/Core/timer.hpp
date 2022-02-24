#pragma once

// Adapted from https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Core/Timer.h

#include "pch.h"

namespace Aspen {
	class Timer {
	public:
		Timer() {
			reset();
		}

		void reset() {
			m_start = std::chrono::high_resolution_clock::now();
		}

		float elapsedSeconds() {
			return std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - m_start).count();
		}

		float elapsedMillis() {
			return elapsedSeconds() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	};
} // namespace Aspen