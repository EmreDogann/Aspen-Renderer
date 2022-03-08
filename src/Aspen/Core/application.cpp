#include "Aspen/Core/application.hpp"

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace Aspen {
	Application::Application() {
		aspenWindow.setEventCallback(BIND_EVENT_FN(Application::OnEvent));

		m_Scene = std::make_shared<Scene>(aspenDevice);

		// Setup runtime camera entity.
		cameraEntity = m_Scene->createEntity("Camera");
		cameraEntity.addComponent<CameraComponent>();

		loadGameObjects();
	}

	void Application::run() {
		std::cout << "maxPushConstantSize = " << aspenDevice.properties.limits.maxPushConstantsSize << "\n";

		// Game Loop
		while (m_Running) {
			aspenWindow.OnUpdate(); // Process window level events.
			// m_Scene->OnUpdate();

			auto frameTime = timer.elapsedSeconds();
			frameTime = glm::min(frameTime, MAX_FRAME_TIME);
			timer.reset();

			// Update the camera position and rotation.
			AspenCamera& camera = cameraEntity.getComponent<CameraComponent>().camera;
			TransformComponent& cameraTransform = cameraEntity.getComponent<TransformComponent>();
			cameraController.moveInPlaneXZ(aspenWindow.getGLFWwindow(), frameTime, cameraTransform);
			camera.setViewYXZ(cameraTransform.translation, cameraTransform.rotation);

			float aspect = aspenRenderer.getAspectRatio();
			// camera.setOrthographicProjection(-1, 1, -1, 1, -1, 1, aspect);
			camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

			if (auto* commandBuffer = aspenRenderer.beginFrame()) {
				aspenRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, m_Scene, camera);
				aspenRenderer.endSwapChainRenderPass(commandBuffer);
				aspenRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(aspenDevice.device()); // Block CPU until all GPU operations have completed.
	}

	void Application::OnEvent(Event& e) {
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

		std::cout << e.ToString() << std::endl;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e) {
		aspenWindow.resetWindowResizedFlag();
		aspenRenderer.recreateSwapChain();
		// createPipeline(); // Right now this is not required as the new render pass will be compatible with the old one but is put here for future proofing.

		// The perspective must be recalculated as the aspect ratio might have changed.
		float aspect = aspenRenderer.getAspectRatio();

		AspenCamera& camera = cameraEntity.getComponent<CameraComponent>().camera;
		camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

		if (auto* commandBuffer = aspenRenderer.beginFrame()) {
			aspenRenderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(commandBuffer, m_Scene, camera);
			aspenRenderer.endSwapChainRenderPass(commandBuffer);
			aspenRenderer.endFrame();
		}
		return true;
	}

	// Temporary helper function, creates a 1x1x1 cube centered at offset
	void createCubeModel(AspenDevice& device, Buffer& bufferManager, glm::vec3 offset, MeshComponent& meshComponent) {
		meshComponent.vertices = {

		    // Left face (white)
		    {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
		    {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
		    {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
		    {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},

		    // Right face (yellow)
		    {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
		    {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
		    {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
		    {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},

		    // Top face (orange, remember y axis points down)
		    {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
		    {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
		    {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
		    {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},

		    // Bottom face (red)
		    {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
		    {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
		    {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
		    {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},

		    // Nose face (blue)
		    {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
		    {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
		    {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
		    {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},

		    // Tail face (green)
		    {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
		    {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
		    {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
		    {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},

		};

		meshComponent.indices = {0, 1, 2, 0, 3, 1, 4, 5, 6, 4, 7, 5, 8, 9, 10, 8, 11, 9, 12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};

		for (auto& v : meshComponent.vertices) {
			v.position += offset;
		}

		bufferManager.makeBuffer(meshComponent.vertices, meshComponent.indices, meshComponent.vertexMemory);
	}

	void Application::loadGameObjects() {
		auto cube = m_Scene->createEntity("Cube");
		auto& cubeTransform = cube.getComponent<TransformComponent>();
		cubeTransform.translation = {0.0f, 0.0f, 2.5f};
		cubeTransform.scale = {0.5f, 0.5f, 0.5f};

		auto& cubeMesh = cube.addComponent<MeshComponent>();
		createCubeModel(aspenDevice, bufferManager, {0.0f, 0.0f, 0.0f}, cubeMesh); // Converts the unique pointer returned from the function to a shared pointer.

		// cube.model = aspenModel;
	}
} // namespace Aspen