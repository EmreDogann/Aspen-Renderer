#include "Aspen/Core/application.hpp"

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace Aspen {
	Application::Application() {
		setupImGui();

		aspenWindow.setEventCallback(BIND_EVENT_FN(Application::OnEvent));

		m_Scene = std::make_shared<Scene>(aspenDevice);

		// Setup runtime camera entity.
		cameraEntity = m_Scene->createEntity("Camera");
		cameraEntity.addComponent<CameraComponent>();

		loadGameObjects();
	}

	Application::~Application() {
		// Cleanup ImGui.
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void Application::setupImGui() {
		// Initalize the core structures for ImGui.
		ImGui::CreateContext();

		// Initialize ImGui for GLFW
		ImGui_ImplGlfw_InitForVulkan(aspenWindow.getGLFWwindow(), true);

		// Initialize ImGui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = aspenDevice.instance();
		init_info.PhysicalDevice = aspenDevice.physicalDevice();
		init_info.Device = aspenDevice.device();
		init_info.Queue = aspenDevice.graphicsQueue();
		init_info.DescriptorPool = aspenDevice.ImGuiDescriptorPool();
		init_info.MinImageCount = aspenRenderer.getSwapChainImageCount();
		init_info.ImageCount = aspenRenderer.getSwapChainImageCount();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, aspenRenderer.getSwapChainRenderPass());

		// Create and upload font textures to GPU memory.
		VkCommandBuffer commandBuffer = aspenDevice.beginSingleTimeCommandBuffers();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		aspenDevice.endSingleTimeCommandBuffers(commandBuffer);

		// Destroy font textures from CPU memory.
		ImGui_ImplVulkan_DestroyFontUploadObjects();
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
			auto [cameraComponent, cameraTransform] = cameraEntity.getComponent<CameraComponent, TransformComponent>();
			cameraController.moveInPlaneXZ(aspenWindow.getGLFWwindow(), frameTime, cameraTransform);
			cameraComponent.camera.setViewYXZ(cameraTransform.translation, cameraTransform.rotation);

			float aspect = aspenRenderer.getAspectRatio();
			// camera.setOrthographicProjection(-1, 1, -1, 1, -1, 1, aspect);
			cameraComponent.camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

			if (auto* commandBuffer = aspenRenderer.beginFrame()) {
				aspenRenderer.beginSwapChainRenderPass(commandBuffer);

				// Start the Dear ImGui frame
				ImGui_ImplVulkan_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
				ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

				ImGui::ShowDemoWindow();

				// Performance Metrics Window
				{
					static int corner = 0;
					static bool p_open = true;

					ImGuiIO& io = ImGui::GetIO();
					ImGuiWindowFlags window_flags =
					    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

					if (corner != -1) {
						const float PAD = 10.0f;
						const ImGuiViewport* viewport = ImGui::GetMainViewport();
						ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
						ImVec2 work_size = viewport->WorkSize;
						ImVec2 window_pos, window_pos_pivot;
						window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
						window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
						window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
						window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
						ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
						window_flags |= ImGuiWindowFlags_NoMove;
					}

					ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

					if (ImGui::Begin("Performance Metrics", &p_open, window_flags)) {
						ImGui::Text("Performance Metrics");
						ImGui::Separator();

						ImGui::Text("Average over 120 frames: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
						ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);

						if (ImGui::BeginPopupContextWindow()) {
							if (ImGui::MenuItem("Custom", nullptr, corner == -1))
								corner = -1;
							if (ImGui::MenuItem("Top-left", nullptr, corner == 0))
								corner = 0;
							if (ImGui::MenuItem("Top-right", nullptr, corner == 1))
								corner = 1;
							if (ImGui::MenuItem("Bottom-left", nullptr, corner == 2))
								corner = 2;
							if (ImGui::MenuItem("Bottom-right", nullptr, corner == 3))
								corner = 3;
							if (p_open && ImGui::MenuItem("Close"))
								p_open = false;
							ImGui::EndPopup();
						}
					}
					ImGui::End();
				}

				ImGui::Render();

				simpleRenderSystem.renderGameObjects(commandBuffer, m_Scene, cameraComponent.camera);

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

		// std::cout << e.ToString() << std::endl;
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

			// Start the Dear ImGui frame
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::ShowDemoWindow();

			static int corner = 0;
			static bool p_open = true;

			ImGuiIO& io = ImGui::GetIO();
			ImGuiWindowFlags window_flags =
			    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

			if (corner != -1) {
				const float PAD = 10.0f;
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
				ImVec2 work_size = viewport->WorkSize;
				ImVec2 window_pos, window_pos_pivot;
				window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
				window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
				window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
				window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
				ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
				window_flags |= ImGuiWindowFlags_NoMove;
			}

			ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

			if (ImGui::Begin("Performance Metrics", &p_open, window_flags)) {
				ImGui::Text("Performance Metrics");
				ImGui::Separator();

				ImGui::Text("Application average over 120 frames: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
				ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices, io.MetricsRenderIndices / 3);

				if (ImGui::BeginPopupContextWindow()) {
					if (ImGui::MenuItem("Custom", nullptr, corner == -1))
						corner = -1;
					if (ImGui::MenuItem("Top-left", nullptr, corner == 0))
						corner = 0;
					if (ImGui::MenuItem("Top-right", nullptr, corner == 1))
						corner = 1;
					if (ImGui::MenuItem("Bottom-left", nullptr, corner == 2))
						corner = 2;
					if (ImGui::MenuItem("Bottom-right", nullptr, corner == 3))
						corner = 3;
					if (p_open && ImGui::MenuItem("Close"))
						p_open = false;
					ImGui::EndPopup();
				}
			}
			ImGui::End();

			ImGui::Render();

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
	}
} // namespace Aspen