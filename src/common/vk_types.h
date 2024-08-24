#pragma once

#include <array>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <fmt/core.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>


template<typename T>
struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    size_t size; // Number of elements

    size_t sizeBytes() { return sizeof(T) * size; }
};

struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

inline void VK_CHECK(VkResult res) {
    if (res != VkResult::VK_SUCCESS) {
        std::cerr << "Vulkan error: " << string_VkResult(res) << std::endl;
        abort();
    }
}

template<typename T>
inline void VKB_CHECK(vkb::Result<T> vkbResult) {
    if (!vkbResult) {
        std::cerr << "VKB error: " <<  vkbResult.error().message() << std::endl;
        abort();
    }
}
