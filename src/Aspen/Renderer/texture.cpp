/*
 * Vulkan texture loader
 *
 * Copyright(C) by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
 */

#include "Aspen/Renderer/texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Aspen {
	void Texture::updateDescriptor() {
		descriptor.sampler = sampler;
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;
	}

	Texture::~Texture() {
		destroy();
	}

	void Texture::destroy() {
		vkDestroyImageView(device->device(), view, nullptr);
		vkDestroyImage(device->device(), image, nullptr);
		if (sampler) {
			vkDestroySampler(device->device(), sampler, nullptr);
		}
		vkFreeMemory(device->device(), deviceMemory, nullptr);
	}

	void Texture::loadImage(std::string filename, ImageProperties& imageProps) {
		if (!VulkanTools::fileExists(filename)) {
			// VulkanTools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
			throw std::runtime_error("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.");
		}

		imageProps.pixels = stbi_load(filename.c_str(), &imageProps.texWidth, &imageProps.texHeight, &imageProps.texChannels, STBI_rgb_alpha);
		if (!imageProps.pixels) {
			throw std::runtime_error("Failed to load texture image at path: " + filename);
		}
	}

	/**
	 * Load a 2D texture including all mip levels
	 *
	 * @param filename File to load
	 * @param format Vulkan format of the image data stored in the file
	 * @param device Vulkan device to create the texture on
	 * @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	 * @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	 * @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	 * @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
	 *
	 */
	void Texture2D::loadFromFile(Device* device, std::string filename, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout, bool forceLinear) {
		ImageProperties imageProps{};
		loadImage(filename, imageProps);

		assert(imageProps.pixels != nullptr);

		this->device = device;
		width = imageProps.texWidth;
		height = imageProps.texHeight;
		mipLevels = 1;

		// Calculate the size of the image in bytes.
		// In the case of a STBI_rgb_alpha, the pixels are laid out row by row with 4 bytes per pixel.
		VkDeviceSize imageSize = width * height * 4;

		// Get device properties for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device->physicalDevice(), format, &formatProperties);

		// Only use linear tiling if requested (and supported by the device).
		// Support for linear tiling is mostly limited, so prefer to use optimal tiling instead.
		// On most implementations linear tiling will only support a very limited amount of formats and features (mip maps, cubemaps, arrays, etc.).
		VkBool32 useStaging = !forceLinear;

		if (useStaging) {
			// Create a host-visible staging buffer that contains the raw pixel data
			// Create staging buffer and allocate memory for it.
			Buffer stagingBuffer{
			    *device,
			    imageSize,
			    1,
			    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			};

			// TODO: Keep the staging buffer mapped and re-use the staging buffers.
			// Copy the pixel data into the staging buffer.
			stagingBuffer.map();
			stagingBuffer.writeToBuffer(imageProps.pixels);

			// Setup buffer copy regions for each mip level
			std::vector<VkBufferImageCopy> bufferCopyRegions;

			for (uint32_t i = 0; i < mipLevels; i++) {
				// ktx_size_t offset;
				// KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
				// assert(result == KTX_SUCCESS);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = std::max(1u, width >> i);
				bufferCopyRegion.imageExtent.height = std::max(1u, height >> i);
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = 0;

				bufferCopyRegions.push_back(bufferCopyRegion);
			}

			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = {width, height, 1};
			imageCreateInfo.usage = imageUsageFlags;

			// Ensure that the TRANSFER_DST bit is set for staging
			if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			// Create and bind an image on GPU local memory.
			device->createImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mipLevels;
			subresourceRange.layerCount = 1;

			VkCommandBuffer commandBuffer = device->beginSingleTimeCommandBuffers();

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy
			VulkanTools::setImageLayout(
			    commandBuffer,
			    image,
			    VK_IMAGE_LAYOUT_UNDEFINED,
			    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			    subresourceRange);

			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
			    commandBuffer,
			    stagingBuffer.getBuffer(),
			    image,
			    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			    static_cast<uint32_t>(bufferCopyRegions.size()),
			    bufferCopyRegions.data());

			// Change texture image layout to shader read after all mip levels have been copied
			this->imageLayout = imageLayout;
			VulkanTools::setImageLayout(
			    commandBuffer,
			    image,
			    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			    imageLayout,
			    subresourceRange);

			device->endSingleTimeCommandBuffers(commandBuffer);

			// Note when the staging buffer variable goes out of scope after here, its destructor will be called and it's resources will be freed.
		} else {
			// Prefer using optimal tiling, as linear tiling
			// may support only a small set of features
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			// Check if this support is supported for linear tiling
			// assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

			// VkImage mappableImage;
			// VkDeviceMemory mappableMemory;

			// VkImageCreateInfo imageCreateInfo{};
			// imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			// imageCreateInfo.format = format;
			// imageCreateInfo.extent = {width, height, 1};
			// imageCreateInfo.mipLevels = 1;
			// imageCreateInfo.arrayLayers = 1;
			// imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			// imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			// imageCreateInfo.usage = imageUsageFlags;
			// imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			// // Load mip map level 0 to linear tiling image
			// VK_CHECK_RESULT(vkCreateImage(device->device(), &imageCreateInfo, nullptr, &mappableImage));

			// // Get memory requirements for this image
			// // like size and alignment
			// vkGetImageMemoryRequirements(device->device(), mappableImage, &memReqs);
			// // Set memory allocation size to required memory size
			// memAllocInfo.allocationSize = memReqs.size;

			// // Get memory type that can be mapped to host memory
			// memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			// // Allocate host memory
			// VK_CHECK_RESULT(vkAllocateMemory(device->device(), &memAllocInfo, nullptr, &mappableMemory));

			// // Bind allocated image for use
			// VK_CHECK_RESULT(vkBindImageMemory(device->device(), mappableImage, mappableMemory, 0));

			// // Get sub resource layout
			// // Mip map count, array layer, etc.
			// VkImageSubresource subRes = {};
			// subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// subRes.mipLevel = 0;

			// VkSubresourceLayout subResLayout;
			// void* data;

			// // Get sub resources layout
			// // Includes row pitch, size offsets, etc.
			// vkGetImageSubresourceLayout(device->device(), mappableImage, &subRes, &subResLayout);

			// // Map image memory
			// VK_CHECK_RESULT(vkMapMemory(device->device(), mappableMemory, 0, memReqs.size, 0, &data));

			// // Copy image data into memory
			// memcpy(data, ktxTextureData, memReqs.size);

			// vkUnmapMemory(device->device(), mappableMemory);

			// // Linear tiled images don't need to be staged
			// // and can be directly used as textures
			// image = mappableImage;
			// deviceMemory = mappableMemory;
			// this->imageLayout = imageLayout;

			// // Setup image memory barrier
			// vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

			// device->flushCommandBuffer(copyCmd, copyQueue);
		}

		// Free the image.
		stbi_image_free(imageProps.pixels);

		// Create image view
		// Textures are not directly accessed by the shaders and are abstracted by image views containing additional information and sub resource ranges.
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		viewCreateInfo.subresourceRange.levelCount = (useStaging) ? mipLevels : 1;
		viewCreateInfo.image = image;
		VK_CHECK_RESULT(vkCreateImageView(device->device(), &viewCreateInfo, nullptr, &view));

		// Create a default sampler
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		// Max level-of-detail should match mip level count
		samplerCreateInfo.maxLod = (useStaging) ? static_cast<float>(mipLevels) : 0.0f;
		// Only enable anisotropic filtering if enabled on the device
		samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
		samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE; // texture coordinates need to be between 0-1.
		VK_CHECK_RESULT(vkCreateSampler(device->device(), &samplerCreateInfo, nullptr, &sampler));

		// Update descriptor image info member that can be used for setting up descriptor sets
		updateDescriptor();
	}

	/**
	 * Creates a 2D texture from a buffer
	 *
	 * @param buffer Buffer containing texture data to upload
	 * @param bufferSize Size of the buffer in machine units
	 * @param width Width of the texture to create
	 * @param height Height of the texture to create
	 * @param format Vulkan format of the image data stored in the file
	 * @param device Vulkan device to create the texture on
	 * @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	 * @param (Optional) filter Texture filtering for the sampler (defaults to VK_FILTER_LINEAR)
	 * @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	 * @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	 */
	void Texture2D::fromBuffer(Device* device, void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, VkQueue copyQueue, VkFilter filter, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout) {
		// assert(buffer);

		// this->device = device;
		// width = texWidth;
		// height = texHeight;
		// mipLevels = 1;

		// VkMemoryAllocateInfo memAllocInfo{};
		// VkMemoryRequirements memReqs;

		// // Use a separate command buffer for texture loading
		// VkCommandBuffer copyCmd = device->beginSingleTimeCommandBuffers();

		// // Create a host-visible staging buffer that contains the raw image data
		// VkBuffer stagingBuffer;
		// VkDeviceMemory stagingMemory;

		// VkBufferCreateInfo bufferCreateInfo{};
		// bufferCreateInfo.size = bufferSize;
		// // This buffer is used as a transfer source for the buffer copy
		// bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// VK_CHECK_RESULT(vkCreateBuffer(device->device(), &bufferCreateInfo, nullptr, &stagingBuffer));

		// // Get memory requirements for the staging buffer (alignment, memory type bits)
		// vkGetBufferMemoryRequirements(device->device(), stagingBuffer, &memReqs);

		// memAllocInfo.allocationSize = memReqs.size;
		// // Get memory type index for a host visible buffer
		// memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// VK_CHECK_RESULT(vkAllocateMemory(device->device(), &memAllocInfo, nullptr, &stagingMemory));
		// VK_CHECK_RESULT(vkBindBufferMemory(device->device(), stagingBuffer, stagingMemory, 0));

		// // Copy texture data into staging buffer
		// uint8_t* data;
		// VK_CHECK_RESULT(vkMapMemory(device->device(), stagingMemory, 0, memReqs.size, 0, (void**)&data));
		// memcpy(data, buffer, bufferSize);
		// vkUnmapMemory(device->device(), stagingMemory);

		// VkBufferImageCopy bufferCopyRegion = {};
		// bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// bufferCopyRegion.imageSubresource.mipLevel = 0;
		// bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		// bufferCopyRegion.imageSubresource.layerCount = 1;
		// bufferCopyRegion.imageExtent.width = width;
		// bufferCopyRegion.imageExtent.height = height;
		// bufferCopyRegion.imageExtent.depth = 1;
		// bufferCopyRegion.bufferOffset = 0;

		// // Create optimal tiled target image
		// VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
		// imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		// imageCreateInfo.format = format;
		// imageCreateInfo.mipLevels = mipLevels;
		// imageCreateInfo.arrayLayers = 1;
		// imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		// imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		// imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// imageCreateInfo.extent = {width, height, 1};
		// imageCreateInfo.usage = imageUsageFlags;
		// // Ensure that the TRANSFER_DST bit is set for staging
		// if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		// 	imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		// }
		// VK_CHECK_RESULT(vkCreateImage(device->device(), &imageCreateInfo, nullptr, &image));

		// vkGetImageMemoryRequirements(device->device(), image, &memReqs);

		// memAllocInfo.allocationSize = memReqs.size;

		// memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		// VK_CHECK_RESULT(vkAllocateMemory(device->device(), &memAllocInfo, nullptr, &deviceMemory));
		// VK_CHECK_RESULT(vkBindImageMemory(device->device(), image, deviceMemory, 0));

		// VkImageSubresourceRange subresourceRange = {};
		// subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// subresourceRange.baseMipLevel = 0;
		// subresourceRange.levelCount = mipLevels;
		// subresourceRange.layerCount = 1;

		// // Image barrier for optimal image (target)
		// // Optimal image will be used as destination for the copy
		// vks::tools::setImageLayout(
		//     copyCmd,
		//     image,
		//     VK_IMAGE_LAYOUT_UNDEFINED,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     subresourceRange);

		// // Copy mip levels from staging buffer
		// vkCmdCopyBufferToImage(
		//     copyCmd,
		//     stagingBuffer,
		//     image,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     1,
		//     &bufferCopyRegion);

		// // Change texture image layout to shader read after all mip levels have been copied
		// this->imageLayout = imageLayout;
		// vks::tools::setImageLayout(
		//     copyCmd,
		//     image,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     imageLayout,
		//     subresourceRange);

		// device->flushCommandBuffer(copyCmd, copyQueue);

		// // Clean up staging resources
		// vkFreeMemory(device->device(), stagingMemory, nullptr);
		// vkDestroyBuffer(device->device(), stagingBuffer, nullptr);

		// // Create sampler
		// VkSamplerCreateInfo samplerCreateInfo = {};
		// samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		// samplerCreateInfo.magFilter = filter;
		// samplerCreateInfo.minFilter = filter;
		// samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		// samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// samplerCreateInfo.mipLodBias = 0.0f;
		// samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		// samplerCreateInfo.minLod = 0.0f;
		// samplerCreateInfo.maxLod = 0.0f;
		// samplerCreateInfo.maxAnisotropy = 1.0f;
		// VK_CHECK_RESULT(vkCreateSampler(device->device(), &samplerCreateInfo, nullptr, &sampler));

		// // Create image view
		// VkImageViewCreateInfo viewCreateInfo = {};
		// viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		// viewCreateInfo.pNext = NULL;
		// viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		// viewCreateInfo.format = format;
		// viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		// viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		// viewCreateInfo.subresourceRange.levelCount = 1;
		// viewCreateInfo.image = image;
		// VK_CHECK_RESULT(vkCreateImageView(device->device(), &viewCreateInfo, nullptr, &view));

		// // Update descriptor image info member that can be used for setting up descriptor sets
		// updateDescriptor();
	}

	/**
	 * Load a 2D texture array including all mip levels
	 *
	 * @param filename File to load
	 * @param format Vulkan format of the image data stored in the file
	 * @param device Vulkan device to create the texture on
	 * @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	 * @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	 * @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	 *
	 */
	void Texture2DArray::loadFromFile(Device* device, std::string filename, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout) {
		// ktxTexture* ktxTexture;
		// ktxResult result = loadKTXFile(filename, &ktxTexture);
		// assert(result == KTX_SUCCESS);

		// this->device = device;
		// width = ktxTexture->baseWidth;
		// height = ktxTexture->baseHeight;
		// layerCount = ktxTexture->numLayers;
		// mipLevels = ktxTexture->numLevels;

		// ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		// ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

		// VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
		// VkMemoryRequirements memReqs;

		// // Create a host-visible staging buffer that contains the raw image data
		// VkBuffer stagingBuffer;
		// VkDeviceMemory stagingMemory;

		// VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
		// bufferCreateInfo.size = ktxTextureSize;
		// // This buffer is used as a transfer source for the buffer copy
		// bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// VK_CHECK_RESULT(vkCreateBuffer(device->device(), &bufferCreateInfo, nullptr, &stagingBuffer));

		// // Get memory requirements for the staging buffer (alignment, memory type bits)
		// vkGetBufferMemoryRequirements(device->device(), stagingBuffer, &memReqs);

		// memAllocInfo.allocationSize = memReqs.size;
		// // Get memory type index for a host visible buffer
		// memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// VK_CHECK_RESULT(vkAllocateMemory(device->device(), &memAllocInfo, nullptr, &stagingMemory));
		// VK_CHECK_RESULT(vkBindBufferMemory(device->device(), stagingBuffer, stagingMemory, 0));

		// // Copy texture data into staging buffer
		// uint8_t* data;
		// VK_CHECK_RESULT(vkMapMemory(device->device(), stagingMemory, 0, memReqs.size, 0, (void**)&data));
		// memcpy(data, ktxTextureData, ktxTextureSize);
		// vkUnmapMemory(device->device(), stagingMemory);

		// // Setup buffer copy regions for each layer including all of its miplevels
		// std::vector<VkBufferImageCopy> bufferCopyRegions;

		// for (uint32_t layer = 0; layer < layerCount; layer++) {
		// 	for (uint32_t level = 0; level < mipLevels; level++) {
		// 		ktx_size_t offset;
		// 		KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, layer, 0, &offset);
		// 		assert(result == KTX_SUCCESS);

		// 		VkBufferImageCopy bufferCopyRegion = {};
		// 		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// 		bufferCopyRegion.imageSubresource.mipLevel = level;
		// 		bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
		// 		bufferCopyRegion.imageSubresource.layerCount = 1;
		// 		bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
		// 		bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
		// 		bufferCopyRegion.imageExtent.depth = 1;
		// 		bufferCopyRegion.bufferOffset = offset;

		// 		bufferCopyRegions.push_back(bufferCopyRegion);
		// 	}
		// }

		// // Create optimal tiled target image
		// VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
		// imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		// imageCreateInfo.format = format;
		// imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		// imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		// imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// imageCreateInfo.extent = {width, height, 1};
		// imageCreateInfo.usage = imageUsageFlags;
		// // Ensure that the TRANSFER_DST bit is set for staging
		// if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		// 	imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		// }
		// imageCreateInfo.arrayLayers = layerCount;
		// imageCreateInfo.mipLevels = mipLevels;

		// VK_CHECK_RESULT(vkCreateImage(device->device(), &imageCreateInfo, nullptr, &image));

		// vkGetImageMemoryRequirements(device->device(), image, &memReqs);

		// memAllocInfo.allocationSize = memReqs.size;
		// memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// VK_CHECK_RESULT(vkAllocateMemory(device->device(), &memAllocInfo, nullptr, &deviceMemory));
		// VK_CHECK_RESULT(vkBindImageMemory(device->device(), image, deviceMemory, 0));

		// // Use a separate command buffer for texture loading
		// VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// // Image barrier for optimal image (target)
		// // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
		// VkImageSubresourceRange subresourceRange = {};
		// subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// subresourceRange.baseMipLevel = 0;
		// subresourceRange.levelCount = mipLevels;
		// subresourceRange.layerCount = layerCount;

		// vks::tools::setImageLayout(
		//     copyCmd,
		//     image,
		//     VK_IMAGE_LAYOUT_UNDEFINED,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     subresourceRange);

		// // Copy the layers and mip levels from the staging buffer to the optimal tiled image
		// vkCmdCopyBufferToImage(
		//     copyCmd,
		//     stagingBuffer,
		//     image,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     static_cast<uint32_t>(bufferCopyRegions.size()),
		//     bufferCopyRegions.data());

		// // Change texture image layout to shader read after all faces have been copied
		// this->imageLayout = imageLayout;
		// vks::tools::setImageLayout(
		//     copyCmd,
		//     image,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     imageLayout,
		//     subresourceRange);

		// device->flushCommandBuffer(copyCmd, copyQueue);

		// // Create sampler
		// VkSamplerCreateInfo samplerCreateInfo = vks::initializers::samplerCreateInfo();
		// samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		// samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		// samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		// samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		// samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		// samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		// samplerCreateInfo.mipLodBias = 0.0f;
		// samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
		// samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
		// samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		// samplerCreateInfo.minLod = 0.0f;
		// samplerCreateInfo.maxLod = (float)mipLevels;
		// samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		// VK_CHECK_RESULT(vkCreateSampler(device->device(), &samplerCreateInfo, nullptr, &sampler));

		// // Create image view
		// VkImageViewCreateInfo viewCreateInfo = vks::initializers::imageViewCreateInfo();
		// viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		// viewCreateInfo.format = format;
		// viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		// viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		// viewCreateInfo.subresourceRange.layerCount = layerCount;
		// viewCreateInfo.subresourceRange.levelCount = mipLevels;
		// viewCreateInfo.image = image;
		// VK_CHECK_RESULT(vkCreateImageView(device->device(), &viewCreateInfo, nullptr, &view));

		// // Clean up staging resources
		// ktxTexture_Destroy(ktxTexture);
		// vkFreeMemory(device->device(), stagingMemory, nullptr);
		// vkDestroyBuffer(device->device(), stagingBuffer, nullptr);

		// // Update descriptor image info member that can be used for setting up descriptor sets
		// updateDescriptor();
	}

	/**
	 * Load a cubemap texture including all mip levels from a single file
	 *
	 * @param filename File to load
	 * @param format Vulkan format of the image data stored in the file
	 * @param device Vulkan device to create the texture on
	 * @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	 * @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	 * @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	 *
	 */
	void TextureCubeMap::loadFromFile(Device* device, std::string filename, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout) {
		// ktxTexture* ktxTexture;
		// ktxResult result = loadKTXFile(filename, &ktxTexture);
		// assert(result == KTX_SUCCESS);

		// this->device = device;
		// width = ktxTexture->baseWidth;
		// height = ktxTexture->baseHeight;
		// mipLevels = ktxTexture->numLevels;

		// ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		// ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

		// VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
		// VkMemoryRequirements memReqs;

		// // Create a host-visible staging buffer that contains the raw image data
		// VkBuffer stagingBuffer;
		// VkDeviceMemory stagingMemory;

		// VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
		// bufferCreateInfo.size = ktxTextureSize;
		// // This buffer is used as a transfer source for the buffer copy
		// bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		// bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// VK_CHECK_RESULT(vkCreateBuffer(device->device(), &bufferCreateInfo, nullptr, &stagingBuffer));

		// // Get memory requirements for the staging buffer (alignment, memory type bits)
		// vkGetBufferMemoryRequirements(device->device(), stagingBuffer, &memReqs);

		// memAllocInfo.allocationSize = memReqs.size;
		// // Get memory type index for a host visible buffer
		// memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// VK_CHECK_RESULT(vkAllocateMemory(device->device(), &memAllocInfo, nullptr, &stagingMemory));
		// VK_CHECK_RESULT(vkBindBufferMemory(device->device(), stagingBuffer, stagingMemory, 0));

		// // Copy texture data into staging buffer
		// uint8_t* data;
		// VK_CHECK_RESULT(vkMapMemory(device->device(), stagingMemory, 0, memReqs.size, 0, (void**)&data));
		// memcpy(data, ktxTextureData, ktxTextureSize);
		// vkUnmapMemory(device->device(), stagingMemory);

		// // Setup buffer copy regions for each face including all of its mip levels
		// std::vector<VkBufferImageCopy> bufferCopyRegions;

		// for (uint32_t face = 0; face < 6; face++) {
		// 	for (uint32_t level = 0; level < mipLevels; level++) {
		// 		ktx_size_t offset;
		// 		KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
		// 		assert(result == KTX_SUCCESS);

		// 		VkBufferImageCopy bufferCopyRegion = {};
		// 		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// 		bufferCopyRegion.imageSubresource.mipLevel = level;
		// 		bufferCopyRegion.imageSubresource.baseArrayLayer = face;
		// 		bufferCopyRegion.imageSubresource.layerCount = 1;
		// 		bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
		// 		bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
		// 		bufferCopyRegion.imageExtent.depth = 1;
		// 		bufferCopyRegion.bufferOffset = offset;

		// 		bufferCopyRegions.push_back(bufferCopyRegion);
		// 	}
		// }

		// // Create optimal tiled target image
		// VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
		// imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		// imageCreateInfo.format = format;
		// imageCreateInfo.mipLevels = mipLevels;
		// imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		// imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		// imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// imageCreateInfo.extent = {width, height, 1};
		// imageCreateInfo.usage = imageUsageFlags;
		// // Ensure that the TRANSFER_DST bit is set for staging
		// if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
		// 	imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		// }
		// // Cube faces count as array layers in Vulkan
		// imageCreateInfo.arrayLayers = 6;
		// // This flag is required for cube map images
		// imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		// VK_CHECK_RESULT(vkCreateImage(device->device(), &imageCreateInfo, nullptr, &image));

		// vkGetImageMemoryRequirements(device->device(), image, &memReqs);

		// memAllocInfo.allocationSize = memReqs.size;
		// memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// VK_CHECK_RESULT(vkAllocateMemory(device->device(), &memAllocInfo, nullptr, &deviceMemory));
		// VK_CHECK_RESULT(vkBindImageMemory(device->device(), image, deviceMemory, 0));

		// // Use a separate command buffer for texture loading
		// VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// // Image barrier for optimal image (target)
		// // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
		// VkImageSubresourceRange subresourceRange = {};
		// subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// subresourceRange.baseMipLevel = 0;
		// subresourceRange.levelCount = mipLevels;
		// subresourceRange.layerCount = 6;

		// vks::tools::setImageLayout(
		//     copyCmd,
		//     image,
		//     VK_IMAGE_LAYOUT_UNDEFINED,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     subresourceRange);

		// // Copy the cube map faces from the staging buffer to the optimal tiled image
		// vkCmdCopyBufferToImage(
		//     copyCmd,
		//     stagingBuffer,
		//     image,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     static_cast<uint32_t>(bufferCopyRegions.size()),
		//     bufferCopyRegions.data());

		// // Change texture image layout to shader read after all faces have been copied
		// this->imageLayout = imageLayout;
		// vks::tools::setImageLayout(
		//     copyCmd,
		//     image,
		//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//     imageLayout,
		//     subresourceRange);

		// device->flushCommandBuffer(copyCmd, copyQueue);

		// // Create sampler
		// VkSamplerCreateInfo samplerCreateInfo = vks::initializers::samplerCreateInfo();
		// samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		// samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		// samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		// samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		// samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		// samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		// samplerCreateInfo.mipLodBias = 0.0f;
		// samplerCreateInfo.maxAnisotropy = device->enabledFeatures.samplerAnisotropy ? device->properties.limits.maxSamplerAnisotropy : 1.0f;
		// samplerCreateInfo.anisotropyEnable = device->enabledFeatures.samplerAnisotropy;
		// samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		// samplerCreateInfo.minLod = 0.0f;
		// samplerCreateInfo.maxLod = (float)mipLevels;
		// samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		// VK_CHECK_RESULT(vkCreateSampler(device->device(), &samplerCreateInfo, nullptr, &sampler));

		// // Create image view
		// VkImageViewCreateInfo viewCreateInfo = vks::initializers::imageViewCreateInfo();
		// viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		// viewCreateInfo.format = format;
		// viewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		// viewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		// viewCreateInfo.subresourceRange.layerCount = 6;
		// viewCreateInfo.subresourceRange.levelCount = mipLevels;
		// viewCreateInfo.image = image;
		// VK_CHECK_RESULT(vkCreateImageView(device->device(), &viewCreateInfo, nullptr, &view));

		// // Clean up staging resources
		// ktxTexture_Destroy(ktxTexture);
		// vkFreeMemory(device->device(), stagingMemory, nullptr);
		// vkDestroyBuffer(device->device(), stagingBuffer, nullptr);

		// // Update descriptor image info member that can be used for setting up descriptor sets
		// updateDescriptor();
	}

} // namespace Aspen
