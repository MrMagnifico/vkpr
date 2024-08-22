#pragma once

#include <stdint.h>


namespace vkCommon {
enum class DrawType { CLEAR, COMPUTE };

// Push constants for compute shaders
struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

// Capture a compute pipeline alongside its push constant parameters
struct ComputeEffect {
    std::string name;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	ComputePushConstants data;
};

struct Config {
    // Draw type control
    DrawType currentDraw = {DrawType::COMPUTE};

    // Fullscreen clear
    glm::vec4 clearColor = {0, 0, 0, 1};

    // Compute background effects
    std::vector<vkCommon::ComputeEffect> backgroundEffects  = {};
    int32_t selectedEffect                                  = {0};
};
}
