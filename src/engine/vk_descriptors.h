#pragma once

#include <common/vk_types.h>

namespace vkEngine {
struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorAllocator {
    struct PoolSizeRatio {
		VkDescriptorType type;
		float ratio;
    };
    VkDescriptorPool pool;

    // State control
    void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clearDescriptors(VkDevice device);
    void destroyPool(VkDevice device);

    // Allocate a set from the pool
    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};
}
