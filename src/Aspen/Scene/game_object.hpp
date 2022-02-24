#pragma once
#include "Aspen/Core/model.hpp"

// Libs
#include "glm/ext/matrix_transform.hpp"
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Aspen {
	struct TransformComponent {
		glm::vec3 translation{}; // Position offset.
		glm::vec3 scale{1.0f, 1.0f, 1.0f};
		glm::vec3 rotation{};

		/*
		    This function calculates and returns the game object's model transformation matrix.
		    The function below forms an affine transformation matrix in the form: translate * Ry & Rx * Rz & scale.
		    The rotation convention follows tait-bryan euler angles with the extrinsic axis order Y(1), X(2), Z(3).
		    Depending on the order you read the rotations, they can be interpreted as either extrinsic or intrinsic rotations.
		    Reading Left-to-Right is Intrinsic.
		    Reading Right-to-Left is Extrinsic.
		    https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		*/
		glm::mat4 mat4() const {
			const float c3 = glm::cos(rotation.z);
			const float s3 = glm::sin(rotation.z);
			const float c2 = glm::cos(rotation.x);
			const float s2 = glm::sin(rotation.x);
			const float c1 = glm::cos(rotation.y);
			const float s1 = glm::sin(rotation.y);
			return glm::mat4{// Column 1
			                 {
			                     scale.x * (c1 * c3 + s1 * s2 * s3),
			                     scale.x * (c2 * s3),
			                     scale.x * (c1 * s2 * s3 - c3 * s1),
			                     0.0f,
			                 },
			                 // Column 2
			                 {
			                     scale.y * (c3 * s1 * s2 - c1 * s3),
			                     scale.y * (c2 * c3),
			                     scale.y * (c1 * c3 * s2 + s1 * s3),
			                     0.0f,
			                 },
			                 // Column 3
			                 {
			                     scale.z * (c2 * s1),
			                     scale.z * (-s2),
			                     scale.z * (c1 * c2),
			                     0.0f,
			                 },
			                 // Column 4
			                 {translation.x, translation.y, translation.z, 1.0f}};
		}
	};

	struct RigidBody2dComponent {
		glm::vec2 velocity;
		float mass{1.0f};
	};

	class AspenGameObject {
	public:
		~AspenGameObject() = default;

		AspenGameObject(const AspenGameObject &) = delete;
		AspenGameObject &operator=(const AspenGameObject &) = delete;

		AspenGameObject(AspenGameObject &&) = default;
		AspenGameObject &operator=(AspenGameObject &&) = default;

		using id_t = unsigned int;

		static AspenGameObject createGameObject() {
			static id_t currentId = 0;
			return AspenGameObject{currentId++};
		}

		id_t getId() const {
			return id;
		};

		std::shared_ptr<AspenModel> model{};
		glm::vec3 color{};
		TransformComponent transform{};
		// RigidBody2dComponent rigidBody2d{};

	private:
		explicit AspenGameObject(id_t objId) : id{objId} {}
		id_t id;
	};
} // namespace Aspen