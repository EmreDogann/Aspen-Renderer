#pragma once
#include "Aspen/Renderer/System/global_render_system.hpp"
#include "Aspen/Scene/entity.hpp"
#include "Aspen/Math/math.hpp"

// Libs
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <imgui_internal.h>
#include "ImGuizmo.h"

namespace Aspen {
	struct UIState {
		bool gizmoVisible = false;
		bool gizmoActive = true;
		int gizmoType = -1;
		bool snapping = false;
		bool viewportHovered = false;
		glm::vec2 viewportSize{};
		glm::vec2 viewportBounds[2]{};
		VkDescriptorSet viewportTexture{};
		Entity selectedEntity{};
	};

	class UIRenderSystem {
	public:
		UIRenderSystem(Device& device, Renderer& renderer, std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout);
		~UIRenderSystem() = default;

		UIRenderSystem(const UIRenderSystem&) = delete;
		UIRenderSystem& operator=(const UIRenderSystem&) = delete;

		UIRenderSystem(UIRenderSystem&&) = delete;            // Move Constructor
		UIRenderSystem& operator=(UIRenderSystem&&) = delete; // Move Assignment Operator

		void render(FrameInfo& frameInfo, UIState& uiState, ApplicationState& appState);
		void onResize();

		VkDescriptorSet getCurrentDescriptorSet(int frameIndex) {
			return descriptorSets[frameIndex];
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();
		void createPipelines();
		void createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout);

		Device& device;
		Renderer& renderer;
		Pipeline pipeline{device};

		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};
		std::vector<VkDescriptorSet> descriptorSets;

		std::vector<std::unique_ptr<Buffer>> uboBuffers;
	};
} // namespace Aspen