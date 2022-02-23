#pragma once

// Libs & defines
#define GLM_FORCE_RADIANS           // Ensures that GLM will expect angles to be specified in radians, not degrees.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Tells GLM to expect depth values in the range 0-1 instead of -1 to 1.
#include <glm/glm.hpp>

// std
#include <cassert>
#include <limits>

namespace Aspen {
	class AspenCamera {
	public:
		void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far, float aspect);
		void setPerspectiveProjection(float fovY, float aspect, float near, float far);

		void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f));
		void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f));
		void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

		const glm::mat4 &getProjection() const {
			return projectionMatrix;
		}

		const glm::mat4 &getView() const {
			return viewMatrix;
		}

	private:
		glm::mat4 projectionMatrix{1.0f};
		glm::mat4 viewMatrix{1.0f};
	};
} // namespace Aspen