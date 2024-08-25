#pragma once

#include <common/constants.h>

#include <glm/mat4x4.hpp>

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
    std::filesystem::path currentPointFile  = vkCommon::constants::RESOURCES_DIR_PATH / "griffin.obj";;
    bool loadNewFile                        = false;

    // Object controls
    float rotationSensitivity   = 0.001f;
    float zoomSensitivity       = 0.01f;
};
}
