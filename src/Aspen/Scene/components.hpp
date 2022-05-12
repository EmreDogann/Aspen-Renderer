#pragma once

#include "Aspen/Renderer/texture.hpp"
#include "Aspen/Renderer/camera.hpp"
#include "Aspen/Core/uuid.hpp"

namespace Aspen {

	struct IDComponent {
		UUID id;
		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent {
		std::string tag;
		TagComponent() = default;
		TagComponent(const TagComponent&) = default;

		TagComponent(std::string tag)
		    : tag(std::move(tag)){};
	};

	struct TransformComponent {
	private:
		glm::mat4 model{1.0f};

	public:
		glm::vec3 translation{0.0f}; // Position offset.
		glm::vec3 scale{1.0f, 1.0f, 1.0f};
		glm::quat rotation{glm::vec3(0.0, 0.0, 0.0)};
		bool isTransformUpdated = false;

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		TransformComponent(const glm::mat4& transform)
		    : model(transform){};

		TransformComponent(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
		    : translation(translation), rotation(glm::quat(rotation)), scale(scale) {
			computeModelMatrix();
		};

		// Returns the transformation matrix.
		operator glm::mat4&() {
			return transform();
		}

		glm::mat4& transform() {
			computeModelMatrix();
			return model;
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
		// void computeModelMatrix() {
		// 	const float c3 = glm::cos(rotation.z);
		// 	const float s3 = glm::sin(rotation.z);
		// 	const float c2 = glm::cos(rotation.x);
		// 	const float s2 = glm::sin(rotation.x);
		// 	const float c1 = glm::cos(rotation.y);
		// 	const float s1 = glm::sin(rotation.y);
		// 	transformMat4 = glm::mat4{// Column 1
		// 	                          {
		// 	                              scale.x * (c1 * c3 + s1 * s2 * s3),
		// 	                              scale.x * (c2 * s3),
		// 	                              scale.x * (c1 * s2 * s3 - c3 * s1),
		// 	                              0.0f,
		// 	                          },
		// 	                          // Column 2
		// 	                          {
		// 	                              scale.y * (c3 * s1 * s2 - c1 * s3),
		// 	                              scale.y * (c2 * c3),
		// 	                              scale.y * (c1 * c3 * s2 + s1 * s3),
		// 	                              0.0f,
		// 	                          },
		// 	                          // Column 3
		// 	                          {
		// 	                              scale.z * (c2 * s1),
		// 	                              scale.z * (-s2),
		// 	                              scale.z * (c1 * c2),
		// 	                              0.0f,
		// 	                          },
		// 	                          // Column 4
		// 	                          {translation.x, translation.y, translation.z, 1.0f}};
		// }

		void computeModelMatrix() {
			glm::mat4 transformMat{1.0f};
			transformMat = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
			model = transformMat;
		}

		glm::mat3 computeNormalMatrix() {
			// Normal matrix = Rotation Matrix * Inverse Scale Matrix
			// We don't care about translation (because normals are directions), so we only return the upper left 3x3 of the matrix.
			return glm::toMat3(rotation) * glm::mat3(glm::scale(glm::mat4(1.0f), 1.0f / scale));
		}
	};

	struct MeshComponent {
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};
			uint32_t materialIndex;

			bool operator==(const Vertex& other) const {
				return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
			}
		};

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::unique_ptr<Buffer> vertexBuffer;
		std::unique_ptr<Buffer> indexBuffer;
		Texture2D texture; // TODO: I need to design a better way to associate textures with objects.

		MeshComponent() = default;
	};

	struct alignas(16) MaterialComponent {
		enum MaterialType {
			Lambertian = 0,
			Metallic = 1,
			Dielectric = 2,
			Isotropic = 3,
			DiffuseLight = 4
		};

		// Note: vec3 and vec4 gets aligned on 16 bytes in Vulkan shaders.

		// Base material.
		glm::vec4 diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		int32_t diffuseTextureId = -1; // -1 means no texture.

		// Metal fuzziness.
		float fuzziness = 1.0f;

		// Shinyness.
		float specular = 1.0f;

		// Dielectric refraction index.
		float refractionIndex = 1.0f;

		// Which material are we dealing with?
		MaterialType materialModel;
	};

	struct CameraComponent {
		Camera camera;
		bool primary = true;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct CameraControllerArcball {
		float moveSpeed{3.0f};
		float orbitSpeed{50.0f};
		float zoomSpeed{0.1f};
		float mouseSensitivity{0.5f};
		glm::vec2 lastMousePosition{0.0f};

		glm::vec3 offset{0.0f, 0.0f, 0.0f};
		glm::vec3 focalPoint{0.0f, 0.0f, 2.5f};
		glm::vec2 positionDisplacement{0.0f};
		float distance = 2.0f;

		float pitch = 0.0f;
		float yaw = 0.0f;

		CameraControllerArcball() = default;
		CameraControllerArcball(const CameraControllerArcball&) = default;
	};

	struct PointLightComponent {
		float lightIntensity = 1.0f;
		glm::vec3 color{1.0f};
	};

	struct RigidBody2dComponent {
		glm::vec2 velocity;
		float mass{1.0f};
	};
} // namespace Aspen