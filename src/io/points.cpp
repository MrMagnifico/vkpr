#include "points.h"

#include <fstream>
#include <iostream>
#include <sstream>


void vkIo::readPointCloud(const char* path, std::vector<Point>& pointcloud, glm::vec3& minimum, glm::vec3& maximum) {
    // Verify file is accessible
    std::ifstream infile(path);
    if (!infile.good()) { throw std::runtime_error("Unable to open point cloud file \"" + std::string(path) + "\""); }

    std::cout << "Reading point cloud..." << std::endl;
    std::string str((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    std::stringstream file(str);
    
    minimum = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    maximum = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    std::string line;
    std::stringstream ss;
    char lineStart;
    Point point;
    while (std::getline(file, line)) {
        ss << line;
        if (ss >> lineStart && lineStart == 'v') {
            if (!(ss >> point.position.x >> point.position.y >> point.position.z)) { continue; }
            if (!(ss >> point.color.x >> point.color.y >> point.color.z)) { point.color = glm::vec3(0.5f, 0.5f, 0.5f); }
            pointcloud.push_back(point);
            minimum = glm::min(point.position, minimum);
            maximum = glm::max(point.position, maximum);
        }
    }
    std::cout << "Done reading!" << std::endl;
}
