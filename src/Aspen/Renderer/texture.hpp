/*
 * Vulkan texture loader
 *
 * Copyright(C) by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once
#include "pch.h"
#include "Aspen/Renderer/buffer.hpp"

namespace Aspen {
	struct ImageProperties {
		int texWidth{}, texHeight{}, texChannels{};
		unsigned char* pixels{};
	};

	class Texture {
	public:
		Device* device;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		VkDescriptorImageInfo descriptor;
		VkSampler sampler;

		~Texture();
		void updateDescriptor();
		void destroy();
		void loadImage(std::string fileName, ImageProperties& imageProps);
	};

	class Texture2D : public Texture {
	public:
		void loadFromFile(
		    Device* device,
		    std::string filename,
		    VkFormat format,
		    VkQueue copyQueue,
		    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    bool forceLinear = false);
		void fromBuffer(
		    Device* device,
		    void* buffer,
		    VkDeviceSize bufferSize,
		    VkFormat format,
		    uint32_t texWidth,
		    uint32_t texHeight,
		    VkQueue copyQueue,
		    VkFilter filter = VK_FILTER_LINEAR,
		    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	};

	class Texture2DArray : public Texture {
	public:
		void loadFromFile(
		    Device* device,
		    std::string filename,
		    VkFormat format,
		    VkQueue copyQueue,
		    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	};

	class TextureCubeMap : public Texture {
	public:
		void loadFromFile(
		    Device* device,
		    std::string filename,
		    VkFormat format,
		    VkQueue copyQueue,
		    VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	};
} // namespace Aspen
