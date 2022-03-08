#pragma once
#include "pch.h"

#include "Aspen/Core/buffer.hpp"
#include "Aspen/Renderer/camera.hpp"

// Libs
#include "glm/ext/matrix_transform.hpp"
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace Aspen {
	struct TagComponent {
		std::string tag;
		TagComponent() = default;
		TagComponent(const TagComponent&) = default;

		TagComponent(std::string tag) : tag(std::move(tag)){};
	};

	struct TransformComponent {
	private:
		glm::mat4 transformMat4{1.0f};

	public:
		glm::vec3 translation{}; // Position offset.
		glm::vec3 scale{1.0f, 1.0f, 1.0f};
		glm::vec3 rotation{};

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		TransformComponent(const glm::mat4& transform) : transformMat4(transform){};

		TransformComponent(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale) : translation(translation), rotation(rotation), scale(scale) {
			computeTransformMatrix();
		};

		// Returns the transformation matrix.
		operator glm::mat4&() {
			return transform();
		}

		glm::mat4& transform() {
			computeTransformMatrix();
			return transformMat4;
		}

		/*
		    This function calculates and returns the game object's model transformation matrix.
		    The function below forms an affine transformation matrix in the form: translate * Ry & Rx * Rz & scale.
		    The rotation convention follows tait-bryan euler angles with the extrinsic axis order Y(1), X(2), Z(3).
		    Depending on the order you read the rotations, they can be interpreted as either extrinsic or intrinsic rotations.
		    Reading Left-to-Right is Intrinsic.
		    Reading Right-to-Left is Extrinsic.
		    https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		*/
		void computeTransformMatrix() {
			const float c3 = glm::cos(rotation.z);
			const float s3 = glm::sin(rotation.z);
			const float c2 = glm::cos(rotation.x);
			const float s2 = glm::sin(rotation.x);
			const float c1 = glm::cos(rotation.y);
			const float s1 = glm::sin(rotation.y);
			transformMat4 = glm::mat4{// Column 1
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

	struct MeshComponent {
		std::vector<Buffer::Vertex> vertices;
		std::vector<uint16_t> indices;
		glm::vec3 color{1.0f};
		VertexMemory vertexMemory{};

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
	};

	struct CameraComponent {
		AspenCamera camera;
		bool primary = true;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;

		// CameraComponent(const glm::mat4& projection) : camera(projection){};
	};

	struct RigidBody2dComponent {
		glm::vec2 velocity;
		float mass{1.0f};
	};
} // namespace Aspen