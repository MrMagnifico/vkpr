#include "vk_descriptors.h"


void vkEngine::DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newBind = {};
    newBind.binding         = binding;
    newBind.descriptorCount = 1;
    newBind.descriptorType  = type;
    bindings.push_back(newBind);
}

void vkEngine::DescriptorLayoutBuilder::clear() { bindings.clear(); }

VkDescriptorSetLayout vkEngine::DescriptorLayoutBuilder::build(VkDevice device,
                                                               VkShaderStageFlags shaderStages,
                                                               void* pNext,
                                                               VkDescriptorSetLayoutCreateFlags flags) {
    // Register desired shader stages with bindings
    for (auto& b : bindings) { b.stageFlags |= shaderStages; }

    // Populate descriptor set layout creation struct with bindings and given flags
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext          = pNext;
    info.pBindings      = bindings.data();
    info.bindingCount   = static_cast<uint32_t>(bindings.size());
    info.flags          = flags;

    // Create and return descriptor set layout
    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
    return set;
}

void vkEngine::DescriptorAllocator::initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
    // Determine the maximum number of allocations of different types that can be owned
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type               = ratio.type,
            .descriptorCount    = static_cast<uint32_t>(ratio.ratio * static_cast<float>(maxSets))
        });
    }

    // Create descriptor pool with desired parameters
	VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags          = 0;
	poolInfo.maxSets        = maxSets;
	poolInfo.poolSizeCount  = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes     = poolSizes.data();
	vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
}

void vkEngine::DescriptorAllocator::clearDescriptors(VkDevice device) { vkResetDescriptorPool(device, pool, 0); }

void vkEngine::DescriptorAllocator::destroyPool(VkDevice device) { vkDestroyDescriptorPool(device, pool, nullptr); }

VkDescriptorSet vkEngine::DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext                 = nullptr;
    allocInfo.descriptorPool        = pool;
    allocInfo.descriptorSetCount    = 1;
    allocInfo.pSetLayouts           = &layout;
    VkDescriptorSet ds;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    return ds;
}
