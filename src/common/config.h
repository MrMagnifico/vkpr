#pragma once

#include <stdint.h>


namespace vkCommon {
enum class DrawType { CLEAR, COMPUTE };

// Push constants for compute shaders
struct ComputePushConstants {
    alignas(16) uint32_t numPoints;
    alignas(16) glm::mat4 mvp;
};

struct Config {
    
};
}
