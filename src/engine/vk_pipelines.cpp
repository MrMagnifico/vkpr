#include "vk_pipelines.h"

#include <engine/vk_initializers.h>

#include <fstream>

bool vkEngine::loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule) {
    // Ppen the file with the cursor at the end
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) { return false; }

    // Find size of file through cursor position (it is at the end)
    size_t fileSize = static_cast<size_t>(file.tellg());

    // SPIR-V expects the buffer to be on uint32, so make sure to reserve a big enough uint32_t vector
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // Load file to buffer
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    // Create a new shader module using buffer we loaded the code into
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext    = nullptr;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t); // codeSize is in bytes
    createInfo.pCode    = buffer.data();
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) { return false; }
    *outShaderModule = shaderModule;
    return true;
}
