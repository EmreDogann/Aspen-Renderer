#include "Aspen/Renderer/System/global_render_system.hpp"

namespace Aspen {
	GlobalRenderSystem::GlobalRenderSystem(Device& device, Renderer& renderer)
	    : device(device), renderer(renderer), uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT), dynamicUboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT), uboDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT), dynamicUboDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT) {
		for (int i = 0; i < uboBuffers.size(); ++i) {
			// Create a UBO buffer. This will just be one instance per frame.
			uboBuffers[i] = std::make_unique<Buffer>(
			    device,
			    sizeof(GlobalUbo),
			    1,
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // We could make it device local but the performance gains could be cancelled out from writing to the UBO every frame.
			    device.properties.limits.minUniformBufferOffsetAlignment);

			// Map the buffer's memory so we can begin writing to it.
			uboBuffers[i]->map();

			// Create a UBO buffer. This will just be one instance per frame.
			dynamicUboBuffers[i] = std::make_unique<Buffer>(
			    device,
			    sizeof(ModelUboDynamic),
			    4,
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // We could make it device local but the performance gains could be cancelled out from writing to the UBO every frame.
			    device.properties.limits.minUniformBufferOffsetAlignment);

			// Map the buffer's memory so we can begin writing to it.
			dynamicUboBuffers[i]->map();
		}

		createDescriptorSetLayout();
		createDescriptorSet();
	}

	// Create a Descriptor Set Layout for a Uniform Buffer Object (UBO) & Textures.
	void GlobalRenderSystem::createDescriptorSetLayout() {
		// Standard UBO
		descriptorSetLayouts.push_back(DescriptorSetLayout::Builder(device)
		                                   .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // Binding 0: Vertex shader uniform buffer
		                                                                                                                                                //   .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)                      // Binding 2: Fragment shader image sampler
		                                   .build());

		// Dynamic UBO
		descriptorSetLayouts.push_back(DescriptorSetLayout::Builder(device)
		                                   .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT) // Binding 0: Vertex shader dynamic uniform buffer
		                                   .build());
	}

	// Create Descriptor Sets.
	void GlobalRenderSystem::createDescriptorSet() {
		// Create descriptor sets for normal UBO.
		for (int i = 0; i < uboDescriptorSets.size(); ++i) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			DescriptorWriter(*descriptorSetLayouts[0], device.getDescriptorPool())
			    .writeBuffer(0, &bufferInfo)
			    // .writeImage(1, &offscreenPass.descriptor)
			    .build(uboDescriptorSets[i]);
		}

		// Create descriptor sets for dynamic UBO.
		for (int i = 0; i < dynamicUboDescriptorSets.size(); ++i) {
			auto dynamicBufferInfo = dynamicUboBuffers[i]->descriptorInfo(dynamicUboBuffers[i]->getAlignmentSize());
			DescriptorWriter(*descriptorSetLayouts[1], device.getDescriptorPool())
			    .writeBuffer(0, &dynamicBufferInfo)
			    .build(dynamicUboDescriptorSets[i]);
		}
	}

	// Update the global UBO.
	void GlobalRenderSystem::updateUBOs(FrameInfo& frameInfo) {
		// Update Global UBO
		{
			GlobalUbo globalUbo{};
			globalUbo.projectionMatrix = frameInfo.camera.getProjection();
			globalUbo.viewMatrix = frameInfo.camera.getView();
			globalUbo.inverseViewMatrix = frameInfo.camera.getInverseView();

			int lightIndex = 0;

			auto group = frameInfo.scene->getPointLights();
			for (const auto& entity : group) {
				assert(lightIndex < MAX_LIGHTS && "Point lights exceeded maximum specified!");

				auto [transform, pointLight] = group.get<TransformComponent, PointLightComponent>(entity);
				// auto rotateLights = glm::rotate(glm::mat4(1.0f), static_cast<float>(frameInfo.frameTime), {0.0f, -1.0f, 0.0f});

				// Update light position
				// 1) Translate the light to the center
				// 2) Make the rotation
				// 3) Translate the object back to its original location
				// auto transformTemp = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.5f));
				// auto translationTemp = (transformTemp * rotateLights * glm::inverse(transformTemp)) * glm::vec4(transform.translation, 1.0f);

				// copy light to ubo
				// ubo.lights[lightIndex].position = translationTemp;
				globalUbo.lights[lightIndex].position = glm::vec4(transform.translation, 1.0f);
				globalUbo.lights[lightIndex].color = glm::vec4(pointLight.color, pointLight.lightIntensity);

				if (++lightIndex == MAX_LIGHTS) {
					break;
				}
			}
			// ubo.numLights = lightIndex;

			uboBuffers[frameInfo.frameIndex]->writeToBuffer(&globalUbo); // Write info to the UBO.
			uboBuffers[frameInfo.frameIndex]->flush();
		}

		// Update Dynamic UBO
		{
			int index = 0;
			auto group = frameInfo.scene->getRenderComponents();
			for (const auto& entity : group) {
				auto [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

				ModelUboDynamic dynamicUbo{};

				// Aligned offset
				// glm::mat4* modelMat = (glm::mat4*)(((uint64_t)dynamicUbo.modelMatrix + (index * uboBuffers[frameInfo.frameIndex]->getAlignmentSize())));

				dynamicUbo.modelMatrix = transform.transform();
				dynamicUbo.normalMatrix = transform.computeNormalMatrix();

				dynamicUboBuffers[frameInfo.frameIndex]->writeToBuffer(&dynamicUbo, sizeof(dynamicUbo), index * dynamicUboBuffers[frameInfo.frameIndex]->getAlignmentSize()); // Write info to the UBO.
				++index;
			}

			dynamicUboBuffers[frameInfo.frameIndex]->flush();
		}
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