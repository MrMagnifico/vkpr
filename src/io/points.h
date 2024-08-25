#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <vector>


namespace vkIo {
constexpr uint64_t TRANSFER_TIMEOUT_NS	= 1000000000; // 1000ms

struct Point {
    glm::vec4 position  = {0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 color     = {0.0f, 0.0f, 0.0f, 1.0f};
};

/**
 * Loads the contents of an .obj file as a point cloud
 * 
 * @param path The path to the file to be loaded
 * @param pointCloud The vector in which the loaded points are stored
 * @param minimum Model minimum bounds
 * @param maximum Model maximum bounds
 */
std::vector<Point> readPointCloud(const std::string path, glm::vec3& minimum, glm::vec3& maximum);
}
