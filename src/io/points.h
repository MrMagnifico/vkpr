#pragma once

#include <glm/vec3.hpp>

namespace vkIo {
constexpr uint64_t TRANSFER_TIMEOUT_NS	= 1000000000; // 1000ms

struct Point {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;
};

/**
 * Loads the contents of an .obj file as a point cloud
 * 
 * @param path The path to the file to be loaded
 * @param pointCloud The vector in which the loaded points are stored
 * @param minimum Model minimum bounds
 * @param maximum Model maximum bounds
 */
void readPointCloud(const char* path, std::vector<Point>& pointCloud, glm::vec3& minimum, glm::vec3& maximum);
}
