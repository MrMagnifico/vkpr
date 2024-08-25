#pragma once

#include <common/constants.h>

#include <filesystem>
#include <stdint.h>


namespace vkCommon {
// Push constants for compute shaders
struct ComputePushConstants {
    alignas(16) uint32_t numPoints;
    alignas(16) glm::mat4 mvp;
};

struct Config {
    // File selection
    std::filesystem::path currentPointFile  = vkCommon::constants::RESOURCES_DIR_PATH / "flowers.obj";;
    bool loadNewFile                        = false;
};
}
