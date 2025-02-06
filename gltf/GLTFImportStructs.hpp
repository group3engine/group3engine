//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_GLTFIMPORTSTRUCTS_HPP
#define VULKANTIME_GLTFIMPORTSTRUCTS_HPP
#include <cstdint>
#include <string>
#include <vector>
struct MeshPrimitive {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    // TODO: Bone weights
    std::vector<std::uint32_t> indices;
    uint32_t meshPrimitiveGPUIndex;
};

struct Mesh {
    std::string name;
    std::vector<MeshPrimitive> meshPrimitives;
};
#endif // VULKANTIME_GLTFIMPORTSTRUCTS_HPP
