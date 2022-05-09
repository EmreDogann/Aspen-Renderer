#include "Aspen/Renderer/camera.hpp"

namespace Aspen {
	// Set projection matrix to be the orthographic projection matrix.
	void Camera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far, float aspect) {
		projectionMatrix = glm::mat4{1.0f};

		// Change dimension scaling if height > width or vice versa.
		if (aspect < 1.0f) {
			top = -1 / aspect;
			bottom = 1 / aspect;
		} else {
			left = -aspect;
			right = aspect;
		}

		projectionMatrix[0][0] = 2.0f / (right - left);
		projectionMatrix[1][1] = 2.0f / (bottom - top);
		projectionMatrix[2][2] = 1.0f / (far - near);
		projectionMatrix[3][0] = -(right + left) / (right - left);
		projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
		projectionMatrix[3][2] = -near / (far - near);

		inverseProjectionMatrix = glm::inverse(projectionMatrix);
	}

	// Set projection matrix to be the perspective projection matrix.
	void Camera::setPerspectiveProjection(float fovY, float aspect, float near, float far) {
		assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

		// Change dimension scaling if height > width or vice versa.
		float aspect_nominator = aspect;
		float aspect_denominator = 1.0f;
		if (aspect < 1.0f) {
			aspect_nominator = 1.0f;
			aspect_denominator = 1 / aspect;
		}

		const float tanHalfFovY = tan(fovY / 2.0f);

		projectionMatrix = glm::mat4{0.0f};

		projectionMatrix[0][0] = 1.0f / (aspect_nominator * tanHalfFovY);
		projectionMatrix[1][1] = 1.0f / (aspect_denominator * tanHalfFovY);
		projectionMatrix[2][2] = far / (far - near);
		projectionMatrix[2][3] = 1.0f;
		projectionMatrix[3][2] = -(far * near) / (far - near);

		inverseProjectionMatrix = glm::inverse(projectionMatrix);
	}

	// The camera view matrix is essentially the inverse of the model transformation matrix used for game objects. view matrix == inverse model matrix.
	// With the model matrix, we move the models from their local object space to world space.
	// With the view matrix we want to move all objects from world space to the camera's 'local object space'. This is why the translation is negative and the rotation is the inverse.

	// Construct a view matrix which will translate the camera back to the origin, and align it's forward direction with the +z axis.
	void Camera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
		// Construct an orthonormal basis.
		// Three vectors of unit length and are all orthogonal (90 degrees) to each other.
		const glm::vec3 w{glm::normalize(direction)};
		const glm::vec3 u{glm::normalize(glm::cross(w, up))};
		const glm::vec3 v{glm::cross(w, u)};

		// Use the orthonormal basis to construct a inverse rotation matrix, combined with an inverse translation matrix.
		// The rotation matrix is special because it's inverse is the same as its transpose (rows become columns and vice versa).
		viewMatrix = glm::mat4{1.0f};
		// Rotation Matrix components
		// Row 1
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;

		// Row 2
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;

		// Row 3
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;

		// Translation matrix components.
		viewMatrix[3][0] = -glm::dot(u, position); // Row 1
		viewMatrix[3][1] = -glm::dot(v, position); // Row 2
		viewMatrix[3][2] = -glm::dot(w, position); // Row 3
	}

	// Construct a view matrix which will align the camera to a position in space.
	void Camera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
		assert((target - position).length() != 0.0f); // Ensure the direction is non-zero.
		setViewDirection(position, target - position, up);
	}

	// Construct a view matrix which will rotate the camera's forward direction with the rotation vector specified.
	// void Camera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
	// 	const float c3 = glm::cos(rotation.z);
	// 	const float s3 = glm::sin(rotation.z);
	// 	const float c2 = glm::cos(rotation.x);
	// 	const float s2 = glm::sin(rotation.x);
	// 	const float c1 = glm::cos(rotation.y);
	// 	const float s1 = glm::sin(rotation.y);

	// 	const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
	// 	const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
	// 	const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};

	// 	// Compute the inverse model matrix.
	// 	viewMatrix = glm::mat4{1.0f};
	// 	// Rotation Matrix components
	// 	// Row 1
	// 	viewMatrix[0][0] = u.x;
	// 	viewMatrix[1][0] = u.y;
	// 	viewMatrix[2][0] = u.z;

	// 	// Row 2
	// 	viewMatrix[0][1] = v.x;
	// 	viewMatrix[1][1] = v.y;
	// 	viewMatrix[2][1] = v.z;

	// 	// Row 3
	// 	viewMatrix[0][2] = w.x;
	// 	viewMatrix[1][2] = w.y;
	// 	viewMatrix[2][2] = w.z;

	// 	// Translation matrix components.
	// 	viewMatrix[3][0] = -glm::dot(u, position); // Row 1
	// 	viewMatrix[3][1] = -glm::dot(v, position); // Row 2
	// 	viewMatrix[3][2] = -glm::dot(w, position); // Row 3
	// }

	// Construct a view matrix which will rotate the camera's forward direction with the rotation vector specified.
	void Camera::setView(glm::vec3 position, glm::vec3 rotation) {
		glm::mat4 transformMat{1.0f};
		transformMat = glm::translate(transformMat, -position);
		transformMat = glm::rotate(transformMat, 0.0f, rotation);
		viewMatrix = glm::inverse(transformMat);
	}

	void Camera::setView(glm::vec3 position, glm::quat rotation) {
		glm::mat4 transformMat{1.0f};
		inverseViewMatrix = glm::translate(transformMat, position) * glm::toMat4(rotation);
		viewMatrix = glm::inverse(inverseViewMatrix);
		// viewMatrix = glm::inverse(glm::toMat4(rotation)) * glm::translate(transformMat, -position);
	}
} // namespace Aspen