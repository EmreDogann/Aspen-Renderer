#include "Aspen/Renderer/System/ui_render_system.hpp"

namespace Aspen {
	struct SimplePushConstantData {
		glm::mat4 modelMatrix{1.0f};
		glm::mat4 normalMatrix{1.0f};
	};

	UIRenderSystem::UIRenderSystem(Device& device, Renderer& renderer, std::vector<std::unique_ptr<DescriptorSetLayout>>& descriptorSetLayout)
	    : device(device), renderer(renderer), uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT), descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT) {

		createPipelineLayout(descriptorSetLayout[0]);
		createPipelines();
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void UIRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 0: Vertex shader uniform buffer
		                                                                                                                                       //   .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)                      // Binding 1: Fragment shader image sampler
		                          .build();
	}

	// Create Descriptor Sets.
	void UIRenderSystem::createDescriptorSet() {
		for (int i = 0; i < descriptorSets.size(); ++i) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			// auto offscreenPass = renderer.getOffscreenPass();
			DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
			    .writeBuffer(0, &bufferInfo)
			    // .writeImage(1, &offscreenPass.descriptor)
			    .build(descriptorSets[i]);
		}
	}

	// Create a pipeline layout.
	void UIRenderSystem::createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Which shaders will have access to this push constant range?
		pushConstantRange.offset = 0;                                                             // To be used if you are using separate ranges for the vertex and fragment shaders.
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{descriptorSetLayout->getDescriptorSetLayout()};
		pipeline.createPipelineLayout(descriptorSetLayouts, pushConstantRange);
	}

	// Create the graphics pipeline defined in aspen_pipeline.cpp
	void UIRenderSystem::createPipelines() {
		assert(pipeline.getPipelineLayout() != nullptr && "Cannot create pipeline before pipeline layout!");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderer.getPresentRenderPass();
		pipelineConfig.pipelineLayout = pipeline.getPipelineLayout();

		pipeline.createShaderModule("assets/shaders/ui_shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		// pipeline.createShaderModule("assets/shaders/ui_shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		pipeline.createGraphicsPipeline(pipelineConfig, pipeline.getPipeline());
	}

	void UIRenderSystem::onResize() {
		// for (int i = 0; i < offscreenDescriptorSets.size(); ++i) {
		// 	auto bufferInfo = uboBuffers[i]->descriptorInfo();
		// 	DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
		// 	    .writeBuffer(0, &bufferInfo)
		// 	    .writeImage(1, &renderer.getOffscreenDescriptorInfo())
		// 	    .overwrite(offscreenDescriptorSets[i]);
		// }

		// VkWriteDescriptorSet offScreenWriteDescriptorSets{};
		// offScreenWriteDescriptorSets.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// offScreenWriteDescriptorSets.dstSet = offscreenDescriptorSets[0];
		// offScreenWriteDescriptorSets.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// offScreenWriteDescriptorSets.dstBinding = 1;
		// offScreenWriteDescriptorSets.descriptorCount = 1;
		// offScreenWriteDescriptorSets.pImageInfo = &renderer.getOffscreenDescriptorInfo();

		// vkUpdateDescriptorSets(device.device(), 1, &offScreenWriteDescriptorSets, 0, nullptr);
	}

	void UIRenderSystem::render(FrameInfo& frameInfo, UIState& uiState, ApplicationState& appState) { // Flush changes to update on the GPU side.
		// Bind the graphics pipieline.
		pipeline.bind(frameInfo.commandBuffer, pipeline.getPipeline());
		vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipelineLayout(), 0, 1, &frameInfo.descriptorSet[0], 0, nullptr);

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
				// ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_right_id);
				ImGui::DockBuilderDockWindow("Settings", dock_right_id);

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

			uiState.viewportHovered = ImGui::IsWindowHovered();

			ImVec2 scenePanelSize = ImGui::GetContentRegionAvail();
			uiState.viewportSize = {scenePanelSize.x, scenePanelSize.y};

			ImGui::Image((ImTextureID)uiState.viewportTexture,
			             ImVec2(static_cast<float>(renderer.getSwapChainExtent().width), static_cast<float>(renderer.getSwapChainExtent().height)));

			ImVec2 windowSize = ImGui::GetWindowSize();
			ImVec2 minBounds = ImGui::GetWindowPos();
			minBounds.x += viewportOffset.x;
			minBounds.y += viewportOffset.y;

			ImVec2 maxBound = {minBounds.x + windowSize.x, minBounds.y + windowSize.y};
			uiState.viewportBounds[0] = {minBounds.x, minBounds.y};
			uiState.viewportBounds[1] = {maxBound.x, maxBound.y};

			uiState.viewportSize = uiState.viewportBounds[1] - uiState.viewportBounds[0];

			// if (uiState.viewportSize.x != renderer.getSwapChainExtent().width || uiState.viewportSize.y != renderer.getSwapChainExtent().height) {
			// 	uiState.viewportResized = true;
			// 	std::cout << "Viewport Size: " << uiState.viewportSize.x << ", " << uiState.viewportSize.y << std::endl;
			// }

			// std::cout << "Min Bounds: " << viewportBounds[0].x << ", " << viewportBounds[0].y << std::endl;
			// std::cout << "Max Bounds: " << viewportBounds[1].x << ", " << viewportBounds[1].y << std::endl;

			if (uiState.gizmoActive) {
				ImGuizmo::Enable(true);
			} else {
				ImGuizmo::Enable(false);
			}

			if (uiState.gizmoVisible && uiState.selectedEntity && uiState.gizmoOperation != -1) {
				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist();

				// Setup ImGuizmo Viewport.
				float windowWidth = ImGui::GetWindowWidth();
				float windowHeight = ImGui::GetWindowHeight();
				ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, static_cast<float>(renderer.getSwapChainExtent().width), static_cast<float>(renderer.getSwapChainExtent().height));

				// Get camera's view and projection matrices.
				const glm::mat4& cameraView = frameInfo.camera.getView();
				glm::mat4 cameraProjection = frameInfo.camera.getProjection();

				// Flip the y axis for perspective projection.
				cameraProjection[1][1] *= -1;

				// Get entity's transform component
				auto& objectTranform = uiState.selectedEntity.getComponent<TransformComponent>().transform();

				glm::vec3 snapValues;
				switch (uiState.gizmoOperation) {
					case ImGuizmo::TRANSLATE:
						snapValues = glm::vec3(uiState.snappingAmounts[0]);
						break;
					case ImGuizmo::ROTATE:
						snapValues = glm::vec3(uiState.snappingAmounts[1]);
						break;
					case ImGuizmo::SCALE:
						snapValues = glm::vec3(uiState.snappingAmounts[2]);
						break;
				}

				// Modify the transform matrix as needed.
				ImGuizmo::Manipulate(glm::value_ptr(cameraView),
				                     glm::value_ptr(cameraProjection),
				                     static_cast<ImGuizmo::OPERATION>(uiState.gizmoOperation),
				                     static_cast<ImGuizmo::MODE>(uiState.gizmoMode),
				                     glm::value_ptr(objectTranform),
				                     nullptr,
				                     uiState.snappingEnabled ? glm::value_ptr(snapValues) : nullptr);

				if (ImGuizmo::IsUsing()) {
					auto& objectComponent = uiState.selectedEntity.getComponent<TransformComponent>();

					// Decompose the transformation matrix into translation, rotation, and scale.
					glm::vec3 translation, rotation, scale;
					Math::decomposeTransform(objectTranform, translation, rotation, scale);

					// Apply the new transformation values.
					objectComponent.translation = translation;
					objectComponent.rotation = glm::normalize(glm::quat(rotation));
					objectComponent.scale = scale;
					objectComponent.isTransformUpdated = true;
				}
			}
			ImGui::End();
		}

		// Settings Window
		{
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove |
			                                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
			ImGuiWindowFlags window_flags2 = ImGuiWindowFlags_MenuBar;

			ImGui::SetNextWindowDockID(dock_right_id, ImGuiCond_Once);
			ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
			ImGui::Begin("Settings", nullptr, window_flags);

			// Most "big" widgets share a common width settings by default. See 'Demo->Layout->Widgets Width' for details.

			// e.g. Use 2/3 of the space for widgets and 1/3 for labels (right align)
			ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.35f);

			// e.g. Leave a fixed amount of width for labels (by passing a negative value), the rest goes to widgets.
			// ImGui::PushItemWidth(ImGui::GetFontSize() * -12);

			// if (ImGui::CollapsingHeader("Widgets")) {
			// 	return;
			// }

			bool& changed = appState.stateChanged;
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Game Loop")) {
				// Game Loop
				{
					changed |= ImGui::Checkbox("Enable VSync", &appState.enable_vsync);
					if (appState.enable_vsync) {
						ImGui::BeginDisabled();
					}

					changed |= ImGui::SliderInt("FPS Cap", &appState.fps_cap, 30, 300, "%i", ImGuiSliderFlags_AlwaysClamp);

					if (appState.enable_vsync) {
						ImGui::EndDisabled();
					}
					changed |= ImGui::SliderInt("Update Rate", &appState.update_rate, 1, 300, "%i", ImGuiSliderFlags_AlwaysClamp);
				}

				// Vsync
				{
					if (!appState.enable_vsync) {
						ImGui::BeginDisabled();
					}

					changed |= ImGui::Checkbox("VSync Snapping", &appState.vsync_snapping);
					{
						if (!appState.vsync_snapping) {
							ImGui::BeginDisabled();
						}

						changed |= ImGui::DragFloat("Vsync Error", &appState.vsync_error, 0.0001f, 0.0f, 0.0f, "%.06f s");

						if (!appState.vsync_snapping) {
							ImGui::EndDisabled();
						}
					}

					if (!appState.enable_vsync) {
						ImGui::EndDisabled();
					}
				}
				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Rendering")) {
				{
					changed |= ImGui::RadioButton("Rasterization", &appState.useRayTracer, 0);
					ImGui::SameLine();
					changed |= ImGui::RadioButton("Ray Tracing", &appState.useRayTracer, 1);

					if (appState.useRayTracer) {
						changed |= ImGui::DragInt("Max Bounces", &appState.rtTotalBounces);
						changed |= ImGui::DragFloat("Min Distance", &appState.rayMinRange, 0.1f, 0.0f, 0.0f, "%.6f");
						changed |= ImGui::Checkbox("Reflections?", &appState.useRTReflections);
						changed |= ImGui::Checkbox("Refractions?", &appState.useRTRefractions);
					}

					changed |= ImGui::Checkbox("Shadow Mapping", &appState.useShadows);
					if (appState.useShadows) {
						float shadowBias;
						float shadowOpacity;
						if (appState.useRayTracer) {
							shadowBias = appState.rtShadowBias;
							shadowOpacity = appState.rtShadowOpacity;
						} else {
							shadowBias = appState.rasterShadowBias;
							shadowOpacity = appState.rasterShadowOpacity;
						}

						changed |= ImGui::DragFloat("Shadow Bias", &shadowBias, 0.0001f, 0.0f, 0.0f, "%.6f");
						changed |= ImGui::DragFloat("Shadow Opcaity", &shadowOpacity, 0.1f, 0.0f, 1.0f, "%.6f");

						if (changed) {
							if (appState.useRayTracer) {
								appState.rtShadowBias = shadowBias;
								appState.rtShadowOpacity = shadowOpacity;
							} else {
								appState.rasterShadowBias = shadowBias;
								appState.rasterShadowOpacity = shadowOpacity;
							}
						}
					}

					changed |= ImGui::Checkbox("Texture Mapping", &appState.useTextureMapping);
				}
				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if ((uiState.gizmoVisible && uiState.selectedEntity) && ImGui::TreeNode("Transforms")) {
				if (ImGui::RadioButton("Translate", uiState.gizmoOperation == ImGuizmo::TRANSLATE)) {
					uiState.gizmoOperation = ImGuizmo::TRANSLATE;
				}

				ImGui::SameLine();

				if (ImGui::RadioButton("Rotate", uiState.gizmoOperation == ImGuizmo::ROTATE)) {
					uiState.gizmoOperation = ImGuizmo::ROTATE;
				}

				ImGui::SameLine();

				if (ImGui::RadioButton("Scale", uiState.gizmoOperation == ImGuizmo::SCALE)) {
					uiState.gizmoOperation = ImGuizmo::SCALE;
				}

				// Get entity's transform component
				auto& objectTranform = uiState.selectedEntity.getComponent<TransformComponent>();

				bool wasValueChanged = false;
				glm::vec3 translation, rotation, scale;
				Math::decomposeTransform(objectTranform, translation, rotation, scale);
				wasValueChanged |= ImGui::DragFloat3("Tr", glm::value_ptr(translation), 0.25f);
				wasValueChanged |= ImGui::DragFloat3("Rt", glm::value_ptr(rotation), 0.25f);
				wasValueChanged |= ImGui::DragFloat3("Sc", glm::value_ptr(scale), 0.25f);

				if (wasValueChanged) {
					objectTranform.translation = translation;
					objectTranform.rotation = glm::normalize(glm::quat(rotation));
					objectTranform.scale = scale;
					objectTranform.isTransformUpdated = true;
				}

				if (uiState.gizmoOperation != ImGuizmo::SCALE) {
					if (ImGui::RadioButton("Local", uiState.gizmoMode == ImGuizmo::LOCAL)) {
						uiState.gizmoMode = ImGuizmo::LOCAL;
					}

					ImGui::SameLine();

					if (ImGui::RadioButton("World", uiState.gizmoMode == ImGuizmo::WORLD)) {
						uiState.gizmoMode = ImGuizmo::WORLD;
					}
				}

				ImGui::Checkbox("Snapping?", &uiState.snappingEnabled);

				ImGui::SameLine();

				switch (uiState.gizmoOperation) {
					case ImGuizmo::TRANSLATE:
						ImGui::InputFloat("Position Snap", &uiState.snappingAmounts[0]);
						break;
					case ImGuizmo::ROTATE:
						ImGui::InputFloat("Angle Snap", &uiState.snappingAmounts[1]);
						break;
					case ImGuizmo::SCALE:
						ImGui::InputFloat("Scale Snap", &uiState.snappingAmounts[2]);
						break;
				}
				ImGui::TreePop();
			}

			ImGui::PopItemWidth();
			ImGui::End();
			// ImGui::ShowDemoWindow();

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
					ImGui::Text("%d vertices, %d indices (%d triangles)", io.MetricsRenderVertices + appState.totalVertexCount, io.MetricsRenderIndices + appState.totalIndexCount, io.MetricsRenderIndices + appState.totalIndexCount / 3);

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
		}

		ImGui::Render();

		// Render ImGui UI at the end of the render pass.
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frameInfo.commandBuffer);
	}
} // namespace Aspen