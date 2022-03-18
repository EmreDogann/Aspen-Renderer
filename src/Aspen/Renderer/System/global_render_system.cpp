#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	GlobalRenderSystem::GlobalRenderSystem(Device& device, Renderer& renderer)
	    : device(device), renderer(renderer), uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT), descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT) {
		for (int i = 0; i < uboBuffers.size(); ++i) {
			// Create a UBO buffer. This will just be one instance per frame.
			uboBuffers[i] = std::make_unique<Buffer>(
			    device,
			    sizeof(GlobalUbo),
			    1,
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); // We could make it device local but the performance gains could be cancelled out from writing to the UBO every frame.

			// Map the buffer's memory so we can begin writing to it.
			uboBuffers[i]->map();
		}
		createDescriptorSetLayout();
		createDescriptorSet();
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void GlobalRenderSystem::createDescriptorSetLayout() {
		descriptorSetLayout = DescriptorSetLayout::Builder(device)
		                          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 0: Vertex shader uniform buffer
		                          .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)                      // Binding 1: Fragment shader image sampler
		                          .build();
	}

	// Create Descriptor Sets.
	void GlobalRenderSystem::createDescriptorSet() {
		for (int i = 0; i < descriptorSets.size(); ++i) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*descriptorSetLayout, device.getDescriptorPool())
			    .writeBuffer(0, &bufferInfo)
			    .writeImage(1, &renderer.getOffscreenDescriptorInfo())
			    .build(descriptorSets[i]);
		}
	}

	// Update the global UBO.
	void GlobalRenderSystem::updateUBOs(FrameInfo& frameInfo, GlobalUbo& ubo) {
		ubo.projectionMatrix = frameInfo.camera.getProjection();
		ubo.viewMatrix = frameInfo.camera.getView();
		ubo.inverseViewMatrix = frameInfo.camera.getInverseView();

		int lightIndex = 0;

		auto group = frameInfo.scene->getPointLights();
		for (auto& entity : group) {
			auto [transform, pointLight] = group.get<TransformComponent, PointLightComponent>(entity);
			auto rotateLights = glm::rotate(glm::mat4(1.0f), frameInfo.frameTime, {0.0f, -1.0f, 0.0f});

			assert(lightIndex < MAX_LIGHTS && "Point lights exceeded maximum specified!");

			// Update light position
			// 1) Translate the light to the center
			// 2) Make the rotation
			// 3) Translate the object back to its original location
			auto transformTemp = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.5f));
			transform.translation = (transformTemp * rotateLights * glm::inverse(transformTemp)) * glm::vec4(transform.translation, 1.0f);

			// copy light to ubo
			ubo.lights[lightIndex].position = glm::vec4(transform.translation, 1.0f);
			ubo.lights[lightIndex].color = glm::vec4(pointLight.color, pointLight.lightIntensity);

			if (++lightIndex == MAX_LIGHTS) {
				break;
			}
		}
		// ubo.numLights = lightIndex;
	}

	// void GlobalRenderSystem::onResize() {
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
	// }
} // namespace Aspen