#pragma once 
#include <common/vk_types.h>

namespace vkEngine {

bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);

};