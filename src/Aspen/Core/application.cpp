#include "Aspen/Core/application.hpp"

#include <thread>

namespace Aspen {
	Application::Application() {
		s_Instance = this;
		window.setEventCallback(BIND_EVENT_FN(Application::OnEvent));

		// Create the mouse picking render pass and tell it to use the attachment from the depth pre-pass.

		setupImGui();

		m_Scene = std::make_shared<Scene>(device);

		uiState.viewportSize = glm::vec2(WIDTH, HEIGHT);
		uiState.selectedEntity.setScene(&*m_Scene);

		// Setup runtime camera entity.
		cameraEntity = m_Scene->createEntity("Camera");
		cameraEntity.addComponent<CameraComponent>();
		cameraEntity.addComponent<CameraControllerArcball>();

		auto& camController = cameraEntity.getComponent<CameraControllerArcball>();
		camController.offset = glm::vec3{0.0f, -1.5f, 0.0f};

		loadEntities();
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
		init_info.DescriptorPool = device.getDescriptorPoolImGui().getDescriptorPool();
		init_info.MinImageCount = renderer.getSwapChainImageCount();
		init_info.ImageCount = renderer.getSwapChainImageCount();
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, renderer.getPresentRenderPass());

		// Create and upload font textures to GPU memory.
		VkCommandBuffer commandBuffer = device.beginSingleTimeCommandBuffers();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		device.endSingleTimeCommandBuffers(commandBuffer);

		std::shared_ptr<Framebuffer> offscreenPass = simpleRenderSystem.getResources();
		uiState.viewportTexture = ImGui_ImplVulkan_AddTexture(offscreenPass->sampler, offscreenPass->attachments[0].view, offscreenPass->attachments[0].description.finalLayout);

		// Destroy font textures from CPU memory.
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void Application::run() {
		std::cout << "maxPushConstantSize = " << device.properties.limits.maxPushConstantsSize << "\n";
		std::cout << "maxMemoryAllocationCount = " << device.properties.limits.maxMemoryAllocationCount << "\n";

		double accumulator = 0.0;
		double averager_residual = 0.0;
		double fpsUpdate = 0.0;
		double frames = 0.0;

		// Game Loop
		while (m_Running) {
			// Calculate the time taken of the previous frame. Clamp between 0 and max frame time.
			double deltaTime = glm::clamp(timer.elapsedSeconds(), 0.0, appState.MAX_FRAME_TIME);
			timer.reset();

			// Vsync time snapping
			if (appState.vsync_snapping) {
				for (double snap : appState.snap_frequencies) {
					if (std::abs(deltaTime - snap) < appState.vsync_error) {
						deltaTime = snap;
						break;
					}
				}
			}

			// Average delta time.
			// appState.time_averager.enqueue(deltaTime);
			// double averager_sum = 0.0;
			// for (int i = 0; i < appState.TIME_HISTORY_COUNT; i++) {
			// 	averager_sum += appState.time_averager.dequeue();
			// }

			// for (int i = 0; i < appState.TIME_HISTORY_COUNT - 1; i++) {
			// 	appState.time_averager[i] = appState.time_averager[i + 1];
			// }
			// appState.time_averager[appState.TIME_HISTORY_COUNT - 1] = deltaTime;

			// double averager_sum = 0.0;
			// for (int i = 0; i < appState.TIME_HISTORY_COUNT; i++) {
			// 	averager_sum += appState.time_averager[i];
			// }

			// TODO: Fix averaging
			// deltaTime = averager_sum / appState.TIME_HISTORY_COUNT;

			// averager_residual += std::fmod(averager_sum, appState.TIME_HISTORY_COUNT);
			// deltaTime += averager_residual / appState.TIME_HISTORY_COUNT;
			// averager_residual = std::fmod(averager_residual, appState.TIME_HISTORY_COUNT);

			accumulator += deltaTime;
			fpsUpdate += deltaTime;
			// frames++;

			// if (fpsUpdate >= fpsUpdateCooldown) {
			// 	std::string newTitle = window.getWindowTitle() + " | Frame Time: " + std::to_string(1000.0 / frames) + ", FPS: " + std::to_string(frames) + ", UPS: " + std::to_string(appState.UPDATE_RATE);
			// 	window.setWindowTitle(newTitle);
			// 	fpsUpdate = 0;
			// 	frames = 0;
			// }

			// Prevents spiral of doom.
			if (accumulator > appState.MAX_FRAME_TIME || appState.stateChanged) {
				std::cout << "Resyncing..." << std::endl;
				appState.resync = true;
			}

			// Resync the timer variables.
			if (appState.resync) {
				accumulator = 0.0;
				deltaTime = appState.FIXED_DELTA_TIME;
				fpsUpdate = 0.0;

				appState.FIXED_DELTA_TIME = 1.0 / static_cast<double>(appState.update_rate);

				if (renderer.getDesiredPresentMode() != !appState.enable_vsync) {
					renderer.setDesiredPresentMode(!appState.enable_vsync); // 0/False is V-sync, 1/True is Mailbox
					renderer.recreateSwapChain();
					mousePickingRenderSystem.onResize();

					std::shared_ptr<Framebuffer> offscreenPass = simpleRenderSystem.getResources();
					uiState.viewportTexture = ImGui_ImplVulkan_UpdateTexture(uiState.viewportTexture, offscreenPass->sampler, offscreenPass->attachments[0].view, offscreenPass->attachments[0].description.finalLayout);
				}

				appState.resync = false;
				appState.stateChanged = false;
			}

			window.OnUpdate(); // Process window level events.

			if (!appState.enable_vsync) { // Unlocked framerate, use interpolation
				double consumedDeltaTime = deltaTime;

				while (accumulator >= appState.FIXED_DELTA_TIME) {
					// Fixed Update
					// ...

					// Cap variable update's dt to not be larger than fixed update, and interleave it (so game state can always get animation frames it needs)
					if (consumedDeltaTime > appState.FIXED_DELTA_TIME) {
						// Variable Update
						update(appState.FIXED_DELTA_TIME);
						consumedDeltaTime -= appState.FIXED_DELTA_TIME;
					}
					accumulator -= appState.FIXED_DELTA_TIME;
				}

				// Render frame
				update(consumedDeltaTime);
				// render(accumulator / appState.FIXED_DELTA_TIME); // Interpolate deltaTime.

				if (fpsUpdate >= 1.0 / static_cast<double>(appState.fps_cap) && fpsUpdate <= appState.MAX_FRAME_TIME) {
					render(accumulator / appState.FIXED_DELTA_TIME); // Interpolate deltaTime.
					fpsUpdate = 0.0;
				}

			} else { // Locked framerate, don't use interpolation
				while (accumulator >= appState.FIXED_DELTA_TIME * appState.UPDATE_MULTIPLICITY) {
					for (int i = 0; i < appState.UPDATE_MULTIPLICITY; ++i) {
						// Fixed Update
						// ...

						// Variable Update
						update(appState.FIXED_DELTA_TIME);

						accumulator -= appState.FIXED_DELTA_TIME;
					}
				}

				// Render frame
				render(1.0);
			}

			// std::cout << "accFrameTime: " << accFrameTime << ", Normalized: " << accFrameTime / (1.0f / FIXED_UPDATE_FPS) << std::endl;
		}

		vkDeviceWaitIdle(device.device()); // Block CPU until all GPU operations have completed.
	}

	void Application::update(double deltaTime) {
		auto [cameraComponent, cameraArcball, cameraTransform] = cameraEntity.getComponent<CameraComponent, CameraControllerArcball, TransformComponent>();
		CameraControllerSystem::OnUpdate(cameraArcball, {renderer.getSwapChainExtent().width, renderer.getSwapChainExtent().height});
		CameraSystem::OnUpdateArcball(cameraTransform, cameraArcball, appState.FIXED_DELTA_TIME);

		float aspect = renderer.getAspectRatio();
		// camera.setOrthographicProjection(-1, 1, -1, 1, -1, 1, aspect);
		cameraComponent.camera.setPerspectiveProjection(glm::radians(65.0f), aspect, 0.1f, 100.0f);
		cameraComponent.camera.setView(cameraTransform.translation, cameraTransform.rotation);

		auto group = m_Scene->getPointLights();
		for (auto& entity : group) {
			auto [transform, pointLight] = group.get<TransformComponent, PointLightComponent>(entity);
			auto rotateLights = glm::rotate(glm::mat4(1.0f), static_cast<float>(deltaTime), {0.0f, -1.0f, 0.0f});

			// Update light position
			// 1) Translate the light to the center
			// 2) Make the rotation
			// 3) Translate the object back to its original location
			auto transformTemp = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.5f));
			transform.translation = (transformTemp * rotateLights * glm::inverse(transformTemp)) * glm::vec4(transform.translation, 1.0f);
		}

		Input::OnUpdate(); // Update input manager state.
	}

	void Application::render(double deltaTime) {
		auto [cameraComponent, cameraArcball, cameraTransform] = cameraEntity.getComponent<CameraComponent, CameraControllerArcball, TransformComponent>();

		if (auto* commandBuffer = renderer.beginFrame()) {
			FrameInfo frameInfo{
			    renderer.getFrameIndex(),
			    static_cast<float>(deltaTime),                                    // Interpolation - Normalized value.
			    globalRenderSystem.getDescriptorSets()[renderer.getFrameIndex()], // Get the global descriptor set of the current frame.
			    commandBuffer,
			    cameraComponent.camera,
			    m_Scene};

			auto& uboBuffers = globalRenderSystem.getUboBuffers();

			// Update our UBO buffer.
			GlobalRenderSystem::GlobalUbo ubo{};
			globalRenderSystem.updateUBOs(frameInfo, ubo);
			uboBuffers[frameInfo.frameIndex]->writeToBuffer(&ubo); // Write info to the UBO.
			uboBuffers[frameInfo.frameIndex]->flush();

			/*
			    Depth Prepass
			*/
			{
				renderer.beginRenderPass(commandBuffer, depthPrePassRenderSystem.prepareRenderInfo());
				depthPrePassRenderSystem.render(frameInfo, uiState.selectedEntity.getEntity());
				renderer.endRenderPass(commandBuffer);
			}

			/*
			    Render Scene to texture - Offscreen rendering
			*/
			{
				renderer.beginRenderPass(commandBuffer, simpleRenderSystem.prepareRenderInfo());
				simpleRenderSystem.render(frameInfo);
				pointLightRenderSystem.render(frameInfo);
				// Only render an outline if an entity has been selected.
				if (uiState.selectedEntity) {
					outlineRenderSystem.render(frameInfo, uiState.selectedEntity.getEntity());
				}
				renderer.endRenderPass(commandBuffer);
			}

			/*
			    Find object id at the mouse cursor by performing depth testing on the depth pre-pass.
			*/
			bool mousePickingRead = false;
			{
				if (mousePicking) {
					mousePicking = false; // Reset bool.
					mousePickingRead = true;

					auto& storageBuffer = mousePickingRenderSystem.getStorageBuffer();
					// Reset the SSBO.
					MousePickingRenderSystem::MousePickingStorageBuffer ssbo{};
					ssbo.objectId = -1;
					storageBuffer->writeToBuffer(&ssbo); // Write data to the SSBO.
					storageBuffer->flush();              // Make buffer data visible to device.

					// auto resources = mousePickingRenderSystem.getMousePickingResources();
					renderer.beginRenderPass(commandBuffer, mousePickingRenderSystem.prepareRenderInfo());
					// renderer.beginRenderPass(commandBuffer, resources.frameBuffer, resources.renderPass, VkRect2D{{0, 0}, renderer.getSwapChainExtent()}, clearValues);
					mousePickingRenderSystem.render(frameInfo);
					renderer.endRenderPass(commandBuffer);
				}
			}

			/*
			    Render UI (also renders scene from texture into a UI window)
			*/
			{
				renderer.beginPresentRenderPass(commandBuffer);
				// simpleRenderSystem.render(frameInfo);
				// pointLightRenderSystem.render(frameInfo);
				uiRenderSystem.render(frameInfo, uiState, appState);
				renderer.endRenderPass(commandBuffer);
			}

			renderer.endFrame();

			if (mousePickingRead) {
				mousePickingRead = false; // Reset bool.

				auto& storageBuffer = mousePickingRenderSystem.getStorageBuffer();

				// std::this_thread::sleep_for(std::chrono::milliseconds(250));

				// Read from our SSBO buffer.

				MousePickingRenderSystem::MousePickingStorageBuffer ssbo{};
				storageBuffer->invalidate();          // Make buffer data visible to host.
				storageBuffer->readFromBuffer(&ssbo); // Read info from the SSBO.

				// auto group = m_Scene->getMetaDataComponents();
				// auto& tagComponent = group.get<TagComponent>(static_cast<entt::entity>(ssbo.objectId));
				// std::cout << "Entity ID: " << ssbo.objectId << ", Tag: " << tagComponent.tag << std::endl;

				if (ssbo.objectId != -1) {
					uiState.selectedEntity.setEntity(static_cast<entt::entity>(ssbo.objectId));
					uiState.gizmoVisible = true;
				} else {
					// std::cout << "Entity ID: " << ssbo.objectId << std::endl;
					uiState.selectedEntity.setEntity(entt::null);
					uiState.gizmoVisible = false;
				}
			}
		}
		Input::OnUpdate(); // Update input manager state.
	}

	void Application::OnEvent(Event& e) {
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

		if (e.GetEventType() == EventType::KeyPressed) {
			auto& keyEvent = dynamic_cast<KeyPressedEvent&>(e);
			if (keyEvent.GetRepeatCount() == 0) {
				// Gizmo Shorcuts
				switch (keyEvent.GetKeyCode()) {
					case Key::LeftAlt:
						cameraEntity.getComponent<CameraControllerArcball>().lastMousePosition = Input::GetMousePosition();
						uiState.gizmoActive = false;
						break;
					case Key::Q:
						uiState.gizmoType = -1;
						break;
					case Key::T:
						uiState.gizmoType = ImGuizmo::OPERATION::TRANSLATE;
						break;
					case Key::R:
						uiState.gizmoType = ImGuizmo::OPERATION::ROTATE;
						break;
					case Key::Y:
						uiState.gizmoType = ImGuizmo::OPERATION::SCALE;
						break;
					case Key::LeftControl:
						uiState.snapping = true;
						break;
					case Key::Escape:
						m_Running = false; // Set flag to close the application.
						break;
				}
			}
		} else if (e.GetEventType() == EventType::KeyReleased) {
			auto& keyEvent = dynamic_cast<KeyReleasedEvent&>(e);
			switch (keyEvent.GetKeyCode()) {
				// Gizmo Shorcuts
				case Key::LeftAlt:
					uiState.gizmoActive = true;
					break;
				case Key::LeftControl:
					uiState.snapping = false;
					break;
			}
		} else if (e.GetEventType() == EventType::MouseButtonPressed) {
			auto& mouseEvent = dynamic_cast<MouseButtonPressedEvent&>(e);
			switch (mouseEvent.GetMouseButton()) {
				// Mouse Picking
				case Mouse::Button0:
					if ((!uiState.gizmoVisible || !ImGuizmo::IsOver()) && uiState.gizmoActive && uiState.viewportHovered) {
						mousePicking = true;
					}
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
		depthPrePassRenderSystem.onResize();
		mousePickingRenderSystem.onResize();

		uiState.viewportSize = glm::vec2(renderer.getSwapChainExtent().width, renderer.getSwapChainExtent().height);

		std::shared_ptr<Framebuffer> offscreenPass = simpleRenderSystem.getResources();
		uiState.viewportTexture = ImGui_ImplVulkan_UpdateTexture(uiState.viewportTexture, offscreenPass->sampler, offscreenPass->attachments[0].view, offscreenPass->attachments[0].description.finalLayout);

		// createPipeline(); // Right now this is not required as the new render pass will be compatible with the old one but is put here for future proofing.

		// The perspective must be recalculated as the aspect ratio might have changed.
		float aspect = renderer.getAspectRatio();

		Camera& camera = cameraEntity.getComponent<CameraComponent>().camera;
		camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 100.0f);

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
	void createCubeModel(Device& device, glm::vec3 offset, MeshComponent& meshComponent) {
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

		Aspen::Model::makeBuffer(device, meshComponent);
	}

	// Temporary helper function, creates a quad.
	void createFloorModel(Device& device, glm::vec3 offset, MeshComponent& meshComponent) {
		meshComponent.vertices = {

		    // Top face (blue, remember y axis points down)
		    // Vertex, Color, Normal
		    {{-.5f, .0f, -.5f}, {0.3f, 0.49f, 0.66f}, {0.0f, -1.0f, 0.0f}},
		    {{.5f, .0f, .5f}, {0.3f, 0.49f, 0.66f}, {0.0f, -1.0f, 0.0f}},
		    {{-.5f, .0f, .5f}, {0.3f, 0.49f, 0.66f}, {0.0f, -1.0f, 0.0f}},
		    {{.5f, .0f, -.5f}, {0.3f, 0.49f, 0.66f}, {0.0f, -1.0f, 0.0f}},

		};

		meshComponent.indices = {0, 1, 2, 0, 3, 1};

		for (auto& v : meshComponent.vertices) {
			v.position += offset;
		}

		Aspen::Model::makeBuffer(device, meshComponent);
	}

	void Application::loadEntities() {
		// Create Floor
		{
			floor = m_Scene->createEntity("Floor");
			auto& floorTransform = floor.getComponent<TransformComponent>();
			floorTransform.translation = {0.0f, 0.0f, 2.5f};
			floorTransform.scale = {5.0f, 5.0f, 5.0f};

			auto& floorMesh = floor.addComponent<MeshComponent>();
			createFloorModel(device,
			                 {0.0f, 0.0f, 0.0f},
			                 floorMesh); // Converts the unique pointer returned from the function to a shared pointer.
		}

		// Create Vases
		{
			object = m_Scene->createEntity("Vase");
			auto& objectTransform = object.getComponent<TransformComponent>();
			objectTransform.translation = {-1.0f, 0.0f, 2.5f};
			objectTransform.scale = {3.0f, 3.0f, 3.0f};

			auto& objectMesh = object.addComponent<MeshComponent>();
			Model::createModelFromFile(device, objectMesh, "assets/models/flat_vase.obj");
			std::cout << "Flat Vase Vertex Count: " << objectMesh.vertices.size() << std::endl;

			Entity object2 = m_Scene->createEntity("Vase2");
			auto& object2Transform = object2.getComponent<TransformComponent>();
			object2Transform.translation = {1.0f, -0.4f, 2.5f};
			object2Transform.scale = {3.0f, 3.0f, 3.0f};

			auto& object2Mesh = object2.addComponent<MeshComponent>();
			Model::createModelFromFile(device, object2Mesh, "assets/models/smooth_vase.obj");
			std::cout << "Smooth Vase Vertex Count: " << object2Mesh.vertices.size() << std::endl;
		}

		// Create cube
		{
			object = m_Scene->createEntity("Cube");
			auto& objectTransform = object.getComponent<TransformComponent>();
			objectTransform.translation = {0.0f, -1.5f, 3.25f};
			objectTransform.rotation = glm::quat(glm::vec3(1.0f, 2.0f, 1.5f));
			objectTransform.scale = glm::vec3(0.5f);

			auto& objectMesh = object.addComponent<MeshComponent>();
			Model::createModelFromFile(device, objectMesh, "assets/models/cube.obj");
			std::cout << "Colored Cube Vertex Count: " << objectMesh.vertices.size() << std::endl;
		}

		// Create point lights
		{
			// https://www.color-hex.com/color-palette/5361
			std::vector<glm::vec3> colors{
			    {1.0f, 0.1f, 0.1f},
			    {0.1f, 0.1f, 1.0f},
			    {0.1f, 1.0f, 0.1f},
			    {1.0f, 1.0f, 0.1f},
			    {0.1f, 1.0f, 1.0f},
			    {1.0f, 1.0f, 1.0f},
			};

			for (int i = 0; i < colors.size(); i++) {
				Entity pointLightEntity = m_Scene->createEntity("PointLight" + std::to_string(i));
				pointLightEntity.addComponent<PointLightComponent>();

				auto [pointLightTransform, pointLightComponent] = pointLightEntity.getComponent<TransformComponent, PointLightComponent>();
				pointLightTransform.translation = glm::vec3{0.0f, -1.0f, 2.5f};
				pointLightComponent.color = colors[i];
				pointLightComponent.lightIntensity = 0.5f;

				auto rotateLight = glm::rotate(pointLightTransform.transform(), (i * glm::two_pi<float>()) / colors.size(), {0.0f, -1.0f, 0.0f});
				pointLightTransform.translation = glm::vec3(rotateLight * glm::vec4{pointLightTransform.translation, 1.0f});
			}
		}
	}
} // namespace Aspen