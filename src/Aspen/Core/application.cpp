#include "Aspen/Core/application.hpp"

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace Aspen {
	Application::Application() {
		aspenWindow.setEventCallback(BIND_EVENT_FN(Application::OnEvent));
		loadGameObjects();
	}

	Application::~Application() = default;

	void Application::run() {
		std::cout << "maxPushConstantSize = " << aspenDevice.properties.limits.maxPushConstantsSize << "\n";

		SimpleRenderSystem simpleRenderSystem{aspenDevice, aspenRenderer.getSwapChainRenderPass()};
		AspenCamera camera{};                                               // Camera instance.
		AspenGameObject viewerObject = AspenGameObject::createGameObject(); // Empty game object to store the transformation of the camera.
		CameraController cameraController{};                                // Input handler for the camera.

		Timer timer{};

		// Game Loop
		while (m_Running) {
			aspenWindow.OnUpdate(); // Process window level events.

			auto frameTime = timer.elapsedSeconds();
			frameTime = glm::min(frameTime, MAX_FRAME_TIME);
			timer.reset();

			// Update the camera position and rotation.
			cameraController.moveInPlaneXZ(aspenWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = aspenRenderer.getAspectRatio();
			// camera.setOrthographicProjection(-1, 1, -1, 1, -1, 1, aspect);
			camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

			if (auto *commandBuffer = aspenRenderer.beginFrame()) {
				aspenRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
				aspenRenderer.endSwapChainRenderPass(commandBuffer);
				aspenRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(aspenDevice.device()); // Block CPU until all GPU operations have completed.
	}

	void Application::OnEvent(Event &e) {
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

		std::cout << e.ToString() << std::endl;
	}

	bool Application::OnWindowClose(WindowCloseEvent &e) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent &e) {
		m_Running = false;
		return true;
	}

	// Temporary helper function, creates a 1x1x1 cube centered at offset
	std::unique_ptr<AspenModel> createCubeModel(AspenDevice &device, glm::vec3 offset) {
		std::vector<AspenModel::Vertex> vertices{

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

		const std::vector<uint16_t> indices = {0, 1, 2, 0, 3, 1, 4, 5, 6, 4, 7, 5, 8, 9, 10, 8, 11, 9, 12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};

		for (auto &v : vertices) {
			v.position += offset;
		}
		return std::make_unique<AspenModel>(device, vertices, indices);
	}

	void Application::loadGameObjects() {
		std::shared_ptr<AspenModel> aspenModel = createCubeModel(aspenDevice, {0.0f, 0.0f, 0.0f}); // Converts the unique pointer returned from the function to a shared pointer.

		auto cube = AspenGameObject::createGameObject();
		cube.model = aspenModel;
		cube.transform.translation = {0.0f, 0.0f, 2.5f};
		cube.transform.scale = {0.5f, 0.5f, 0.5f};
		gameObjects.push_back(std::move(cube));
	}
} // namespace Aspen