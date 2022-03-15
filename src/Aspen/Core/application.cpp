#include "Aspen/Core/application.hpp"
#include <iostream>

// #define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)
#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Aspen {
	Application* Application::s_Instance = nullptr;

	Application::Application() {
		s_Instance = this;
		window.setEventCallback(BIND_EVENT_FN(Application::OnEvent));
		setupImGui();

		m_Scene = std::make_shared<Scene>(device);

		// Setup runtime camera entity.
		cameraEntity = m_Scene->createEntity("Camera");
		cameraEntity.addComponent<CameraComponent>();
		cameraEntity.addComponent<CameraControllerArcball>();

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

		// Enable docking.
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Initialize ImGui for GLFW
		ImGui_ImplGlfw_InitForVulkan(window.getGLFWwindow(), true);

		// Initialize ImGui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = device.instance();
		init_info.PhysicalDevice = device.physicalDevice();
		init_info.Device = device.device();
		init_info.Queue = device.graphicsQueue();
		init_info.DescriptorPool = device.ImGuiDescriptorPool();
		init_info.MinImageCount = renderer.getSwapChainImageCount();
		init_info.ImageCount = renderer.getSwapChainImageCount();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, renderer.getPresentRenderPass());

		// Create and upload font textures to GPU memory.
		VkCommandBuffer commandBuffer = device.beginSingleTimeCommandBuffers();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		device.endSingleTimeCommandBuffers(commandBuffer);

		SwapChain::OffscreenPass offscreenPass = renderer.getOffscreenPass();
		viewportTexture = ImGui_ImplVulkan_AddTexture(offscreenPass.sampler, offscreenPass.color.view, offscreenPass.descriptor.imageLayout);

		// Destroy font textures from CPU memory.
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void Application::run() {
		std::cout << "maxPushConstantSize = " << device.properties.limits.maxPushConstantsSize << "\n";

		// Game Loop
		while (m_Running) {
			// Calculate the time taken of the previous frame.
			currentFrameTime = timer.elapsedSeconds();
			currentFrameTime = glm::min(currentFrameTime, MAX_FRAME_TIME);
			timer.reset();

			window.OnUpdate(); // Process window level events.
			// m_Scene->OnUpdate();

			// Update the camera position and rotation.
			auto [cameraComponent, cameraArcball, cameraTransform] = cameraEntity.getComponent<CameraComponent, CameraControllerArcball, TransformComponent>();
			CameraControllerSystem::OnUpdate(cameraArcball, {renderer.getSwapChainExtent().width, renderer.getSwapChainExtent().height});
			CameraSystem::OnUpdateArcball(cameraTransform, cameraArcball, currentFrameTime);
			// cameraController.OnUpdate(cameraTransform, currentFrameTime, {renderer.getSwapChainExtent().width, renderer.getSwapChainExtent().height});
			cameraComponent.camera.setView(cameraTransform.translation, cameraTransform.rotation);

			float aspect = renderer.getAspectRatio();
			// camera.setOrthographicProjection(-1, 1, -1, 1, -1, 1, aspect);
			cameraComponent.camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

			if (auto* commandBuffer = renderer.beginFrame()) {

				/*
				    Render Scene to texture - Offscreen rendering
				*/
				renderer.beginOffscreenRenderPass(commandBuffer);

				simpleRenderSystem.renderGameObjects(commandBuffer, m_Scene, cameraComponent.camera);

				renderer.endRenderPass(commandBuffer);

				/*
				    Render UI (render scene from texture into a UI window)
				*/
				renderer.beginPresentRenderPass(commandBuffer);

				// simpleRenderSystem.renderGameObjects(commandBuffer, m_Scene, cameraComponent.camera);
				simpleRenderSystem.renderUI(commandBuffer);
				renderUI(commandBuffer, cameraComponent.camera);

				renderer.endRenderPass(commandBuffer);
				renderer.endFrame();
			}

			// Update input manager state.
			Input::OnUpdate();
		}

		vkDeviceWaitIdle(device.device()); // Block CPU until all GPU operations have completed.
	}

	void Application::renderUI(VkCommandBuffer commandBuffer, Camera camera) {
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		ImGuiID dock_left_id = ImGui::GetID("Viewport");
		ImGuiID dock_right_id = ImGui::GetID("Settings");
		// Base Dockspace
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // Disable window padding
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);            // Disable window border
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);              // Disable window rounding

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGui::Begin("Docking Window",
			             nullptr,
			             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus |
			                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking |
			                 ImGuiWindowFlags_NoFocusOnAppearing);

			ImGui::PopStyleVar(3);

			ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_NoTabBar;
			ImGuiID dockspaceID = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);

			static bool firstTime = true;
			if (firstTime) {
				firstTime = false;

				ImGui::DockBuilderRemoveNode(dockspaceID);
				ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_None);

				ImGuiID dock_main_id = dockspaceID;
				dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.85f, nullptr, &dock_main_id);
				dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.15f, nullptr, &dock_main_id);

				ImGui::DockBuilderDockWindow("Viewport", dock_left_id);
				ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_right_id);
				// ImGui::DockBuilderDockWindow("Settings", dock_right_id);

				// Disable tab bar for custom toolbar
				// ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_right_id);
				// node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

				ImGui::DockBuilderFinish(dockspaceID);
			}

			ImGui::End();
		}

		// Viewport
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Disable window padding
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);         // Disable window border

			ImGui::SetNextWindowDockID(dock_left_id, ImGuiCond_Once);
			ImGui::Begin("Viewport",
			             nullptr,
			             ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove |
			                 ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

			ImGui::PopStyleVar(2);
			ImVec2 viewportOffset = ImGui::GetCursorPos();

			bool sceneHovered = ImGui::IsWindowHovered();

			ImVec2 scenePanelSize = ImGui::GetContentRegionAvail();
			viewportSize = {scenePanelSize.x, scenePanelSize.y};

			ImGui::Image((ImTextureID)viewportTexture,
			             ImVec2(static_cast<float>(renderer.getSwapChainExtent().width), static_cast<float>(renderer.getSwapChainExtent().height)));

			ImVec2 windowSize = ImGui::GetWindowSize();
			ImVec2 minBounds = ImGui::GetWindowPos();
			minBounds.x += viewportOffset.x;
			minBounds.y += viewportOffset.y;

			ImVec2 maxBound = {minBounds.x + windowSize.x, minBounds.y + windowSize.y};
			viewportBounds[0] = {minBounds.x, minBounds.y};
			viewportBounds[1] = {maxBound.x, maxBound.y};

			viewportSize = viewportBounds[1] - viewportBounds[0];

			// std::cout << "Min Bounds: " << viewportBounds[0].x << ", " << viewportBounds[0].y << std::endl;
			// std::cout << "Max Bounds: " << viewportBounds[1].x << ", " << viewportBounds[1].y << std::endl;

			if (object && gizmoType != -1) {
				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist();

				// Setup ImGuizmo Viewport.
				float windowWidth = ImGui::GetWindowWidth();
				float windowHeight = ImGui::GetWindowHeight();
				ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

				// Get camera's view and projection matrices.
				const glm::mat4& cameraView = camera.getView();
				glm::mat4 cameraProjection = camera.getProjection();

				// Flip the y axis for perspective projection.
				cameraProjection[1][1] *= -1;

				// Get entity's transform component
				auto& objectTranform = object.getComponent<TransformComponent>().transform();

				// Snapping
				float snapValue = 0.5f; // Snap to 0.5m for translation/scale.

				// Snap to 45 degrees for rotation.
				if (gizmoType == ImGuizmo::OPERATION::ROTATE) {
					snapValue = 45.0f;
				}

				float snapValues[3] = {snapValue, snapValue, snapValue};

				// Modify the transform matrix as needed.
				ImGuizmo::Manipulate(glm::value_ptr(cameraView),
				                     glm::value_ptr(cameraProjection),
				                     static_cast<ImGuizmo::OPERATION>(gizmoType),
				                     ImGuizmo::LOCAL,
				                     glm::value_ptr(objectTranform),
				                     nullptr,
				                     snapping ? snapValues : nullptr);

				if (ImGuizmo::IsUsing()) {
					auto& objectComponent = object.getComponent<TransformComponent>();

					// Decompose the transformation matrix into translation, rotation, and scale.
					glm::vec3 translation, rotation, scale;
					Math::decomposeTransform(objectTranform, translation, rotation, scale);

					// Apply the new transformation values.
					objectComponent.translation = translation;
					objectComponent.rotation = glm::normalize(glm::quat(rotation));
					objectComponent.scale = scale;
				}
			}
			ImGui::End();
		}

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove |
		                                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav;
		ImGuiWindowFlags window_flags2 = ImGuiWindowFlags_MenuBar;

		ImGui::SetNextWindowDockID(dock_right_id, ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
		// ImGui::Begin("Settings", nullptr, window_flags);
		// ImGuiIO& io = ImGui::GetIO();
		// ImGui::Text("Performance Metrics");
		// ImGui::Separator();

		// ImGui::Text("Average over 120 frames: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		// ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices, io.MetricsRenderIndices,
		// io.MetricsRenderIndices / 3); ImGui::End();
		ImGui::ShowDemoWindow();

		// Performance Metrics Window
		{
			static int corner = 0;
			static bool p_open = true;

			ImGuiIO& io = ImGui::GetIO();
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
			                                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

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
				ImGui::SetNextWindowSize(ImVec2(450.0f, 500.0f), ImGuiCond_FirstUseEver);
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

		// Render ImGui UI at the end of the render pass.
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	void Application::OnEvent(Event& e) {
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

		// cameraController.OnEvent(e);

		if (e.GetEventType() == EventType::KeyPressed) {
			auto& keyEvent = dynamic_cast<KeyPressedEvent&>(e);
			// Gizmo Shorcuts
			switch (keyEvent.GetKeyCode()) {
				case Key::Q:
					gizmoType = -1;
					break;
				case Key::T:
					gizmoType = ImGuizmo::OPERATION::TRANSLATE;
					break;
				case Key::R:
					gizmoType = ImGuizmo::OPERATION::ROTATE;
					break;
				case Key::Y:
					gizmoType = ImGuizmo::OPERATION::SCALE;
					break;
				case Key::LeftControl:
					snapping = true;
					break;
			}
		} else if (e.GetEventType() == EventType::KeyReleased) {
			auto& keyEvent = dynamic_cast<KeyReleasedEvent&>(e);
			switch (keyEvent.GetKeyCode()) {
				// Gizmo Shorcuts
				case Key::LeftControl:
					snapping = false;
					break;
			}
		}
		// std::cout << e.ToString() << std::endl;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e) {
		window.resetWindowResizedFlag();
		renderer.recreateSwapChain();
		simpleRenderSystem.onResize();

		SwapChain::OffscreenPass offscreenPass = renderer.getOffscreenPass();
		viewportTexture =
		    ImGui_ImplVulkan_UpdateTexture(viewportTexture, offscreenPass.sampler, offscreenPass.color.view, offscreenPass.descriptor.imageLayout);

		// createPipeline(); // Right now this is not required as the new render pass will be compatible with the old one but is put here for future proofing.

		// The perspective must be recalculated as the aspect ratio might have changed.
		float aspect = renderer.getAspectRatio();
		std::cout << aspect << std::endl;

		Camera& camera = cameraEntity.getComponent<CameraComponent>().camera;
		camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

		// if (auto* commandBuffer = renderer.beginFrame()) {
		// 	renderer.beginRenderPass(commandBuffer);

		// 	simpleRenderSystem.renderGameObjects(commandBuffer, m_Scene, camera);

		// 	renderUI(commandBuffer, camera);

		// 	renderer.endRenderPass(commandBuffer);
		// 	renderer.endFrame();
		// }

		return true;
	}

	// Temporary helper function, creates a 1x1x1 cube centered at offset
	void createCubeModel(Device& device, Buffer& bufferManager, glm::vec3 offset, MeshComponent& meshComponent) {
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

		bufferManager.makeBuffer(device, meshComponent);
	}

	// Temporary helper function, creates a quad.
	void createFloorModel(Device& device, Buffer& bufferManager, glm::vec3 offset, MeshComponent& meshComponent) {
		meshComponent.vertices = {

		    // Top face (orange, remember y axis points down)
		    {{-.5f, .0f, -.5f}, {.9f, .6f, .1f}},
		    {{.5f, .0f, .5f}, {.9f, .6f, .1f}},
		    {{-.5f, .0f, .5f}, {.9f, .6f, .1f}},
		    {{.5f, .0f, -.5f}, {.9f, .6f, .1f}},

		};

		meshComponent.indices = {0, 1, 2, 0, 3, 1};

		for (auto& v : meshComponent.vertices) {
			v.position += offset;
		}

		bufferManager.makeBuffer(device, meshComponent);
	}

	void Application::loadGameObjects() {
		floor = m_Scene->createEntity("Floor");
		auto& floorTransform = floor.getComponent<TransformComponent>();
		floorTransform.translation = {0.0f, 0.0f, 2.5f};
		floorTransform.scale = {2.0f, 2.0f, 2.0f};

		auto& floorMesh = floor.addComponent<MeshComponent>();
		createFloorModel(device,
		                 bufferManager,
		                 {0.0f, 0.0f, 0.0f},
		                 floorMesh); // Converts the unique pointer returned from the function to a shared pointer.

		object = m_Scene->createEntity("Vase");
		auto& objectTransform = object.getComponent<TransformComponent>();
		objectTransform.translation = {0.0f, 0.0f, 2.5f};
		// objectTransform.scale = glm::vec3(3.0f); // Uniform scaling
		objectTransform.scale = {3.0f, 1.0f, 3.0f}; // Non-Uniform scaling

		auto& objectMesh = object.addComponent<MeshComponent>();
		Buffer::createModelFromFile(device, objectMesh, "assets/models/flat_vase.obj");
		std::cout << "Vertex Count: " << objectMesh.vertices.size() << std::endl;
	}
} // namespace Aspen