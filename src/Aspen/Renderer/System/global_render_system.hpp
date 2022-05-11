#pragma once
#include "Aspen/Renderer/pipeline.hpp"
#include "Aspen/Renderer/renderer.hpp"
#include "Aspen/Renderer/framebuffer.hpp"

namespace Aspen {
	class RenderSystem {
	public:
		virtual void createResources() = 0;
		virtual void render(FrameInfo& frameInfo) = 0;
		virtual void onResize() = 0;

		virtual void createDescriptorSetLayout() = 0;
		virtual void createDescriptorSet() = 0;

		virtual void createPipeline() = 0;
		virtual void createPipelineLayout(std::unique_ptr<DescriptorSetLayout>& globalDescriptorSetLayout) = 0;

		virtual RenderInfo prepareRenderInfo() = 0;
	};

	static const int MAX_LIGHTS = 10;

	class GlobalRenderSystem {
	public:
		struct PointLight {
			glm::vec4 position{};
			glm::vec4 color{}; // w is intensity
		};

		struct GlobalUbo {
			glm::mat4 projectionMatrix{1.0f};
			glm::mat4 viewMatrix{1.0f};
			glm::mat4 inverseProjectionMatrix{1.0f};
			glm::mat4 inverseViewMatrix{1.0f};
			PointLight lights[MAX_LIGHTS];
			alignas(16) glm::vec3 ambientLightColor{0.1f};
			// alignas(4) int numLights;
		};

		struct ModelUboDynamic {
			glm::mat4 modelMatrix;
			glm::mat4 normalMatrix;
		};

		GlobalRenderSystem(Device& device, Renderer& renderer);
		~GlobalRenderSystem() = default;

		GlobalRenderSystem(const GlobalRenderSystem&) = delete;
		GlobalRenderSystem& operator=(const GlobalRenderSystem&) = delete;

		GlobalRenderSystem(GlobalRenderSystem&&) = delete;            // Move Constructor
		GlobalRenderSystem& operator=(GlobalRenderSystem&&) = delete; // Move Assignment Operator

		// void render(FrameInfo& frameInfo) override;
		// void onResize() override;
		void updateUBOs(FrameInfo& frameInfo);

		std::vector<std::unique_ptr<DescriptorSetLayout>>& getDescriptorSetLayout() {
			return descriptorSetLayouts;
		}

		std::vector<VkDescriptorSet>& getUboDescriptorSets() {
			return uboDescriptorSets;
		}

		std::vector<VkDescriptorSet>& getDynamicUboDescriptorSets() {
			return dynamicUboDescriptorSets;
		}

		std::vector<std::unique_ptr<Buffer>>& getUboBuffers() {
			return uboBuffers;
		}

		std::vector<std::unique_ptr<Buffer>>& getDynamicUboBuffers() {
			return dynamicUboBuffers;
		}

	private:
		void createDescriptorSetLayout();
		void createDescriptorSet();

		Device& device;
		Renderer& renderer;

		std::vector<std::unique_ptr<DescriptorSetLayout>> descriptorSetLayouts{};
		std::vector<VkDescriptorSet> uboDescriptorSets;
		std::vector<VkDescriptorSet> dynamicUboDescriptorSets;

		std::vector<std::unique_ptr<Buffer>> uboBuffers;
		std::vector<std::unique_ptr<Buffer>> dynamicUboBuffers;
	};
} // namespace Aspen