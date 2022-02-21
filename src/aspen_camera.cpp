#include "aspen_camera.hpp"

namespace Aspen {
	// Set projection matrix to be the orthographic projection matrix.
	void AspenCamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
		projectionMatrix = glm::mat4{1.0f};

		projectionMatrix[0][0] = 2.0f / (right - left);
		projectionMatrix[1][1] = 2.0f / (bottom - top);
		projectionMatrix[2][2] = 1.0f / (far - near);
		projectionMatrix[3][0] = -(right + left) / (right - left);
		projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
		projectionMatrix[3][2] = -near / (far - near);
	}

	// Set projection matrix to be the perspective projection matrix.
	void AspenCamera::setPerspectiveProjection(float fovY, float aspect, float near, float far) {
		assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
		const float tanHalfFovY = tan(fovY / 2.0f);

		projectionMatrix = glm::mat4{0.0f};

		projectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovY);
		projectionMatrix[1][1] = 1.0f / (tanHalfFovY);
		projectionMatrix[2][2] = far / (far - near);
		projectionMatrix[2][3] = 1.0f;
		projectionMatrix[3][2] = -(far * near) / (far - near);
	}

	// Construct a view matrix which will translate the camera back to the origin, and align it's forward direction with the +z axis.
	void AspenCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
		// Construct an orthonormal basis.
		// Three vectors of unit length and are all orthogonal (90 degrees) to each other.
		const glm::vec3 w{glm::normalize(direction)};
		const glm::vec3 u{glm::normalize(glm::cross(w, up))};
		const glm::vec3 v{glm::cross(w, u)};

		// Use the orthonormal basis to construct a rotation matrix, combined with a translation matrix.
		viewMatrix = glm::mat4{1.0f};
		// Rotation Matrix components
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;

		// Translation matrix components.
		viewMatrix[3][0] = -glm::dot(u, position);
		viewMatrix[3][1] = -glm::dot(v, position);
		viewMatrix[3][2] = -glm::dot(w, position);
	}

	// Construct a view matrix which will align the camera to a position in space.
	void AspenCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
		assert((target - position).length() != 0.0f); // Ensure the direction is non-zero.
		setViewDirection(position, target - position, up);
	}

	// Construct a view matrix which will rotate the camera's forward direction with the rotation vector specified.
	void AspenCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
		const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
		const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
		viewMatrix = glm::mat4{1.0f};
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;
		viewMatrix[3][0] = -glm::dot(u, position);
		viewMatrix[3][1] = -glm::dot(v, position);
		viewMatrix[3][2] = -glm::dot(w, position);
	}
} // namespace Aspen