#pragma once
#include "pch.h"

// Libs
#include "vulkan/vulkan_core.h"

namespace Aspen {
	class Device;

	class DescriptorSetLayout {
	public:
		class Builder {
		public:
			Builder(Device& device)
			    : device{device} {}

			Builder& addBinding(
			    uint32_t binding,
			    VkDescriptorType descriptorType,
			    VkShaderStageFlags stageFlags,
			    VkDescriptorBindingFlags descriptorBindingFlags = 0,
			    uint32_t count = 1);
			std::unique_ptr<DescriptorSetLayout> build() const;

		private:
			Device& device;
			std::unordered_map<uint32_t, std::tuple<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlags>> bindings{};
		};

		DescriptorSetLayout(
		    Device& device,
		    std::unordered_map<uint32_t, std::tuple<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlags>> bindings);
		~DescriptorSetLayout();
		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const {
			return descriptorSetLayout;
		}

	private:
		Device& device;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, std::tuple<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlags>> bindings;

		friend class DescriptorWriter;
	};

	class DescriptorPool {
	public:
		class Builder {
		public:
			Builder(Device& device)
			    : device{device} {}

			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& setMaxSets(uint32_t count);
			std::unique_ptr<DescriptorPool> build() const;

		private:
			Device& device;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		DescriptorPool(
		    Device& device,
		    uint32_t maxSets,
		    VkDescriptorPoolCreateFlags poolFlags,
		    const std::vector<VkDescriptorPoolSize>& poolSizes);
		~DescriptorPool();
		DescriptorPool(const DescriptorPool&) = delete;
		DescriptorPool& operator=(const DescriptorPool&) = delete;

		VkDescriptorPool getDescriptorPool() const {
			return descriptorPool;
		}

		bool allocateDescriptorSet(
		    const VkDescriptorSetLayout descriptorSetLayout,
		    VkDescriptorSet& descriptorSet,
		    uint32_t variableDescriptorCount = 1) const;

		void freeDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets) const;

		void resetPool();

	private:
		Device& device;
		VkDescriptorPool descriptorPool;

		friend class DescriptorWriter;
	};

	class DescriptorWriter {
	public:
		DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

		DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount = 1);
		DescriptorWriter& writeAccelerationStructure(uint32_t binding, VkWriteDescriptorSetAccelerationStructureKHR* ASInfo);

		bool build(VkDescriptorSet& set);
		void overwrite(VkDescriptorSet& set);

	private:
		DescriptorSetLayout& setLayout;
		DescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};

} // namespace Aspen