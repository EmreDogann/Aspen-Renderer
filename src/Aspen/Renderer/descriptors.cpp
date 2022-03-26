#include "Aspen/Renderer/descriptors.hpp"
#include "Aspen/Renderer/device.hpp"

namespace Aspen {

	// *************** Descriptor Set Layout Builder *********************

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(
	    uint32_t binding,
	    VkDescriptorType descriptorType,
	    VkShaderStageFlags stageFlags,
	    uint32_t count) {
		// Check that binding at the specified index in the map has not already been added.
		assert(bindings.count(binding) == 0 && "Binding already in use");

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;
		bindings[binding] = layoutBinding;

		return *this;
	}

	std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const {
		return std::make_unique<DescriptorSetLayout>(device, bindings);
	}

	// *************** Descriptor Set Layout *********************

	DescriptorSetLayout::DescriptorSetLayout(
	    Device& device,
	    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
	    : device{device}, bindings{bindings} {

		// Vulkan only cares about the descriptor set layout bindings. So we turn our map into a vector of those.
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
		for (auto kv : bindings) {
			setLayoutBindings.push_back(kv.second);
		}

		// Create a set layout.
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(
		        device.device(),
		        &descriptorSetLayoutInfo,
		        nullptr,
		        &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	DescriptorSetLayout::~DescriptorSetLayout() {
		// Cleanup descriptor set layout.
		vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
	}

	// *************** Descriptor Pool Builder *********************

	DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(
	    VkDescriptorType descriptorType,
	    uint32_t count) {
		poolSizes.push_back({descriptorType, count});
		return *this;
	}

	DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(
	    VkDescriptorPoolCreateFlags flags) {
		poolFlags = flags;
		return *this;
	}
	DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(uint32_t count) {
		maxSets = count;
		return *this;
	}

	std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const {
		return std::make_unique<DescriptorPool>(device, maxSets, poolFlags, poolSizes);
	}

	// *************** Descriptor Pool *********************

	DescriptorPool::DescriptorPool(
	    Device& device,
	    uint32_t maxSets,
	    VkDescriptorPoolCreateFlags poolFlags,
	    const std::vector<VkDescriptorPoolSize>& poolSizes)
	    : device{device} {
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if (vkCreateDescriptorPool(device.device(), &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool!");
		}
	}

	DescriptorPool::~DescriptorPool() {
		// Cleanup Descriptor Pool.
		vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
	}

	// Allocates a descriptor set from the descriptor pool.
	bool DescriptorPool::allocateDescriptorSet(
	    const VkDescriptorSetLayout descriptorSetLayout,
	    VkDescriptorSet& descriptorSet) const {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		return vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptorSet) == VK_SUCCESS;
	}

	void DescriptorPool::freeDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets) const {
		vkFreeDescriptorSets(
		    device.device(),
		    descriptorPool,
		    static_cast<uint32_t>(descriptorSets.size()),
		    descriptorSets.data());
	}

	void DescriptorPool::resetPool() {
		vkResetDescriptorPool(device.device(), descriptorPool, 0);
	}

	// *************** Descriptor Writer *********************

	DescriptorWriter::DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool)
	    : setLayout{setLayout}, pool{pool} {}

	DescriptorWriter& DescriptorWriter::writeBuffer(
	    uint32_t binding,
	    VkDescriptorBufferInfo* bufferInfo) {
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = setLayout.bindings[binding];

		assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::writeImage(
	    uint32_t binding,
	    VkDescriptorImageInfo* imageInfo,
	    uint32_t descriptorCount) {
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = setLayout.bindings[binding];

		// assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = descriptorCount;

		writes.push_back(write);
		return *this;
	}

	bool DescriptorWriter::build(VkDescriptorSet& set) {
		bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
		if (!success) {
			return false;
		}
		overwrite(set);
		return true;
	}

	void DescriptorWriter::overwrite(VkDescriptorSet& set) {
		for (auto& write : writes) {
			write.dstSet = set;
		}
		vkUpdateDescriptorSets(pool.device.device(), writes.size(), writes.data(), 0, nullptr);
	}

} // namespace Aspen
