/*
 * Vulkan framebuffer class
 *
 * Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once
#include "pch.h"

#include "Aspen/Renderer/device.hpp"
#include "Aspen/Renderer/tools.hpp"

// Custom define for better code readability
#define VK_FLAGS_NONE 0

namespace Aspen {
	/**
	 * @brief Encapsulates a single frame buffer attachment
	 */
	struct FramebufferAttachment {
		VkImage image{};
		VkDeviceMemory memory{};
		VkImageView view{};
		VkFormat format{};
		VkImageSubresourceRange subresourceRange{};
		VkAttachmentDescription description{};

		/**
		 * @brief Returns true if the attachment has a depth component
		 */
		bool hasDepth() {
			std::vector<VkFormat> formats =
			    {
			        VK_FORMAT_D16_UNORM,
			        VK_FORMAT_X8_D24_UNORM_PACK32,
			        VK_FORMAT_D32_SFLOAT,
			        VK_FORMAT_D16_UNORM_S8_UINT,
			        VK_FORMAT_D24_UNORM_S8_UINT,
			        VK_FORMAT_D32_SFLOAT_S8_UINT,
			    };
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		 * @brief Returns true if the attachment has a stencil component
		 */
		bool hasStencil() {
			std::vector<VkFormat> formats =
			    {
			        VK_FORMAT_S8_UINT,
			        VK_FORMAT_D16_UNORM_S8_UINT,
			        VK_FORMAT_D24_UNORM_S8_UINT,
			        VK_FORMAT_D32_SFLOAT_S8_UINT,
			    };
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		 * @brief Returns true if the attachment is a depth and/or stencil attachment
		 */
		bool isDepthStencil() {
			return (hasDepth() || hasStencil());
		}
	};

	/**
	 * @brief Describes the attributes of an attachment to be created
	 */
	struct AttachmentCreateInfo {
		uint32_t width, height;
		uint32_t layerCount = 1;
		VkFormat format{};
		VkImageUsageFlags usage{};
		VkAttachmentLoadOp loadOp{};
		VkAttachmentStoreOp storeOp{};
		VkAttachmentLoadOp stencilLoadOp{};
		VkAttachmentStoreOp stencilStoreOp{};
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
		VkImageCreateFlags flags{};
	};

	/**
	 * @brief Describes the attributes of an attachment to be added
	 */
	struct AttachmentAddInfo {
		uint32_t width, height;
		uint32_t layerCount;
		VkFormat format;
		VkAttachmentLoadOp loadOp;
		VkAttachmentStoreOp storeOp;
		VkAttachmentLoadOp stencilLoadOp;
		VkAttachmentStoreOp stencilStoreOp;
		VkImageUsageFlags usage;
		VkImageLayout initialLayout;
		VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
		VkImageView view;
	};

	/**
	 * @brief Encapsulates a complete Vulkan framebuffer with an arbitrary number and combination of attachments
	 */
	struct Framebuffer {
	private:
		Device& device;

	public:
		uint32_t width, height;
		VkFramebuffer framebuffer{};
		VkRenderPass renderPass{};
		VkSampler sampler{};
		std::vector<FramebufferAttachment> attachments{};

		/**
		 * Default constructor
		 *
		 * @param device Pointer to a valid apsen device instance
		 */
		Framebuffer(Device& device)
		    : device(device) {}

		/**
		 * Destroy and free Vulkan resources used for the framebuffer and all of its attachments
		 */
		~Framebuffer() {
			for (auto attachment : attachments) {
				// Only destroy attachments if they are not empty (attachments may just contain loaded data from another Framebuffer).
				if (attachment.image) {
					vkDestroyImage(device.device(), attachment.image, nullptr);
					vkDestroyImageView(device.device(), attachment.view, nullptr);
					vkFreeMemory(device.device(), attachment.memory, nullptr);
				}
			}
			vkDestroySampler(device.device(), sampler, nullptr);
			vkDestroyRenderPass(device.device(), renderPass, nullptr);
			vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
		}

		/**
		 * Will use the framebuffer data from an already existing framebuffer.
		 * To be used when multiple systems share the same render pass.
		 *
		 * @param frameBuffer A reference to the framebuffer to be looaded.
		 */
		void loadFramebuffer(Framebuffer& frameBuffer) {
		}

		/**
		 * Will empty the framebuffer, allowing for safe recreation.
		 */
		void clearFramebuffer() {
			for (auto attachment : attachments) {
				// Only destroy attachments if they are not empty (attachments may just contain loaded data from another Framebuffer).
				if (attachment.image) {
					vkDestroyImage(device.device(), attachment.image, nullptr);
					vkDestroyImageView(device.device(), attachment.view, nullptr);
					vkFreeMemory(device.device(), attachment.memory, nullptr);
				}
			}
			attachments.clear();

			vkDestroySampler(device.device(), sampler, nullptr);
			vkDestroyRenderPass(device.device(), renderPass, nullptr);
			vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
		}

		/**
		 * Create a new attachment described by createinfo to the framebuffer's attachment list.
		 * This will also create a image and image view to go with the attachment.
		 *
		 * @param createinfo Structure that specifies the framebuffer to be constructed
		 *
		 * @return Index of the new attachment
		 */
		uint32_t createAttachment(AttachmentCreateInfo createinfo) {
			FramebufferAttachment attachment;

			// TODO: Is there a better way to save the width and height for the Framebuffer?
			width = createinfo.width;
			height = createinfo.height;
			attachment.format = createinfo.format;

			VkImageAspectFlags aspectMask = VK_FLAGS_NONE;

			// Select aspect mask and layout depending on usage

			// Color attachment
			if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			// Depth (and/or stencil) attachment
			if (createinfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				if (attachment.hasDepth()) {
					aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				if (attachment.hasStencil()) {
					aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}

			assert(aspectMask > 0);

			VkImageCreateInfo image{};
			image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image.imageType = VK_IMAGE_TYPE_2D;
			image.format = createinfo.format;
			image.extent.width = createinfo.width;
			image.extent.height = createinfo.height;
			image.extent.depth = 1;
			image.mipLevels = 1;
			image.arrayLayers = createinfo.layerCount;
			image.samples = createinfo.imageSampleCount;
			image.tiling = VK_IMAGE_TILING_OPTIMAL;
			image.usage = createinfo.usage;
			image.flags = createinfo.flags;

			VkMemoryAllocateInfo memAlloc{};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs{};

			// Create image for this attachment
			VK_CHECK(vkCreateImage(device.device(), &image, nullptr, &attachment.image));
			vkGetImageMemoryRequirements(device.device(), attachment.image, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = device.findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK(vkAllocateMemory(device.device(), &memAlloc, nullptr, &attachment.memory));
			VK_CHECK(vkBindImageMemory(device.device(), attachment.image, attachment.memory, 0));

			attachment.subresourceRange = {};
			attachment.subresourceRange.aspectMask = aspectMask;
			attachment.subresourceRange.levelCount = 1;
			attachment.subresourceRange.layerCount = createinfo.layerCount;

			VkImageViewCreateInfo imageView{};
			imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageView.viewType = (createinfo.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : (createinfo.layerCount == 6) ? VK_IMAGE_VIEW_TYPE_CUBE
			                                                                                                         : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			imageView.format = createinfo.format;
			imageView.subresourceRange = attachment.subresourceRange;
			imageView.image = attachment.image;
			VK_CHECK(vkCreateImageView(device.device(), &imageView, nullptr, &attachment.view));

			// Fill attachment description
			attachment.description = {};
			attachment.description.samples = createinfo.imageSampleCount;
			attachment.description.loadOp = createinfo.loadOp;
			// attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.storeOp = createinfo.storeOp;
			attachment.description.stencilLoadOp = createinfo.stencilLoadOp;
			attachment.description.stencilStoreOp = createinfo.stencilStoreOp;
			attachment.description.format = createinfo.format;
			attachment.description.initialLayout = createinfo.initialLayout;
			// Final layout
			// If not, final layout depends on attachment type
			if (attachment.hasDepth() || attachment.hasStencil()) {
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			} else {
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		/**
		 * Add a load attachment described by createinfo to the framebuffer's attachment list.
		 * This will not create an image or image view. Suitable for using attachments/images from previous render passes.
		 *
		 * @param addInfo Structure that specifies the framebuffer to be constructed
		 *
		 * @return Index of the new attachment
		 */
		uint32_t addLoadAttachment(AttachmentAddInfo addInfo) {
			FramebufferAttachment attachment;

			width = addInfo.width;
			height = addInfo.height;
			attachment.format = addInfo.format;
			attachment.view = addInfo.view;

			VkImageAspectFlags aspectMask = VK_FLAGS_NONE;

			// Select aspect mask and layout depending on usage

			// Color attachment
			if (addInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			// Depth (and/or stencil) attachment
			if (addInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				if (attachment.hasDepth()) {
					aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				if (attachment.hasStencil()) {
					aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}

			attachment.subresourceRange = {};
			attachment.subresourceRange.aspectMask = aspectMask;
			attachment.subresourceRange.levelCount = 1;
			attachment.subresourceRange.layerCount = addInfo.layerCount;

			assert(aspectMask > 0);

			// Fill attachment description
			attachment.description = {};
			attachment.description.samples = addInfo.imageSampleCount;
			// attachment.description.loadOp = (addInfo.usage & (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
			// attachment.description.storeOp = (addInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.loadOp = addInfo.loadOp;
			attachment.description.storeOp = addInfo.storeOp;
			attachment.description.stencilLoadOp = addInfo.stencilLoadOp;
			attachment.description.stencilStoreOp = addInfo.stencilStoreOp;
			attachment.description.format = addInfo.format;
			attachment.description.initialLayout = addInfo.initialLayout;

			// Final layout
			// If not, final layout depends on attachment type
			if (attachment.hasDepth() || attachment.hasStencil()) {
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			} else {
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		/**
		 * Creates a default sampler for sampling from any of the framebuffer attachments
		 * Applications are free to create their own samplers for different use cases
		 *
		 * @param magFilter Magnification filter for lookups
		 * @param minFilter Minification filter for lookups
		 * @param adressMode Addressing mode for the U,V and W coordinates
		 *
		 * @return VkResult for the sampler creation
		 */
		VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode) {
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = magFilter;
			samplerInfo.minFilter = minFilter;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = adressMode;
			samplerInfo.addressModeV = adressMode;
			samplerInfo.addressModeW = adressMode;
			samplerInfo.unnormalizedCoordinates = false;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.compareEnable = VK_TRUE;
			samplerInfo.compareOp = VK_COMPARE_OP_LESS;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			return vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler);
		}

		/**
		 * Creates a default render pass setup with one sub pass
		 *
		 * @return VK_SUCCESS if all resources have been created successfully
		 */
		VkResult createRenderPass() {
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (auto& attachment : attachments) {
				attachmentDescriptions.push_back(attachment.description);
			};

			// Collect attachment references
			std::vector<VkAttachmentReference> colorReferences;
			VkAttachmentReference depthReference{};
			bool hasDepth = false;
			bool hasColor = false;

			uint32_t attachmentIndex = 0;

			for (auto& attachment : attachments) {
				if (attachment.isDepthStencil()) {
					// Only one depth attachment allowed
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					hasDepth = true;
				} else {
					colorReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
					hasColor = true;
				}
				attachmentIndex++;
			};

			// Default render pass setup uses only one subpass
			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			if (hasColor) {
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth) {
				subpass.pDepthStencilAttachment = &depthReference;
			}

			// Use subpass dependencies for attachment layout transitions
			std::array<VkSubpassDependency, 2> dependencies{};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Create render pass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass));

			std::vector<VkImageView> attachmentViews;
			for (auto attachment : attachments) {
				attachmentViews.push_back(attachment.view);
			}

			// Find. max number of layers across attachments
			uint32_t maxLayers = 0;
			for (auto attachment : attachments) {
				if (attachment.subresourceRange.layerCount > maxLayers) {
					maxLayers = attachment.subresourceRange.layerCount;
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = maxLayers;
			VK_CHECK(vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffer));

			return VK_SUCCESS;
		}

		/**
		 * Creates a default multi-view render pass setup with one sub pass
		 *
		 * @return VK_SUCCESS if all resources have been created successfully
		 */
		VkResult createMultiViewRenderPass(uint32_t multiViewCount) {
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (auto& attachment : attachments) {
				attachmentDescriptions.push_back(attachment.description);
			};

			// Collect attachment references
			std::vector<VkAttachmentReference> colorReferences;
			VkAttachmentReference depthReference{};
			bool hasDepth = false;
			bool hasColor = false;

			uint32_t attachmentIndex = 0;

			for (auto& attachment : attachments) {
				if (attachment.isDepthStencil()) {
					// Only one depth attachment allowed
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					hasDepth = true;
				} else {
					colorReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
					hasColor = true;
				}
				attachmentIndex++;
			};

			// Default render pass setup uses only one subpass
			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			if (hasColor) {
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth) {
				subpass.pDepthStencilAttachment = &depthReference;
			}

			// Use subpass dependencies for attachment layout transitions
			std::array<VkSubpassDependency, 2> dependencies{};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Create render pass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();

			/*
			    Setup multiview info for the renderpass
			*/

			/*
			    Bit mask that specifies which view rendering is broadcast to
			    0011 = Broadcast to first and second view (layer)
			*/
			const uint32_t viewMask = 0b00111111;

			/*
			    Bit mask that specifies correlation between views
			    An implementation may use this for optimizations (concurrent render)
			*/
			const uint32_t correlationMask = 0b00111111;

			VkRenderPassMultiviewCreateInfo renderPassMultiviewCreateInfo{};
			renderPassMultiviewCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
			renderPassMultiviewCreateInfo.subpassCount = 1;
			renderPassMultiviewCreateInfo.pViewMasks = &viewMask;
			// renderPassMultiviewCreateInfo.correlationMaskCount = 1;
			// renderPassMultiviewCreateInfo.pCorrelationMasks = &correlationMask;

			renderPassInfo.pNext = &renderPassMultiviewCreateInfo;
			VK_CHECK(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass));

			std::vector<VkImageView> attachmentViews;
			for (auto attachment : attachments) {
				attachmentViews.push_back(attachment.view);
			}

			// Find. max number of layers across attachments
			uint32_t maxLayers = 0;
			for (auto attachment : attachments) {
				if (attachment.subresourceRange.layerCount > maxLayers) {
					maxLayers = attachment.subresourceRange.layerCount;
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = 1;
			VK_CHECK(vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &framebuffer));

			return VK_SUCCESS;
		}
	};
} // namespace Aspen