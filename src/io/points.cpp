#include "points.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <format>
#include <iostream>


std::vector<vkIo::Point> vkIo::readPointCloud(const std::string path, glm::vec3& minimum, glm::vec3& maximum) {
    // Read raw OBJ file with tinyobjloader
    // If the file does not have vertex colors, default values are used
    std::cout << "Loading OBJ file..." << std::endl;
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.triangulate = false;
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, readerConfig)) {
        if (!reader.Error().empty()) { throw std::runtime_error(std::format("TinyObjReader: {}", reader.Error())); }
    }
    if (!reader.Warning().empty()) { std::cout << "TinyObjReader: " << reader.Warning();}

    // Read position and color data into return vector and compute minimum and maximum bounds
    std::cout << "Processing loaded OBJ file..." << std::endl;
    Point newPoint;
    std::vector<vkIo::Point> points;
    auto& attributes    = reader.GetAttrib();
    minimum             = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    maximum             = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (size_t vertexIdx = 0; vertexIdx < attributes.vertices.size(); vertexIdx += 3) {
        // Read point data
        newPoint.position   = glm::vec4(attributes.vertices[vertexIdx],
                                        attributes.vertices[vertexIdx + 1],
                                        attributes.vertices[vertexIdx + 2],
                                        1.0f);
        newPoint.color      = glm::vec4(attributes.colors[vertexIdx],
                                        attributes.colors[vertexIdx + 1],
                                        attributes.colors[vertexIdx + 2],
                                        1.0f);
        points.push_back(newPoint);

        // Compute bounds
        minimum = glm::min(glm::vec3(newPoint.position), minimum);
        maximum = glm::max(glm::vec3(newPoint.position), maximum);
    }

    std::cout << "Done loading point data!" << std::endl;
    return points;
}
