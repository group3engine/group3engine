//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_GLTFIMPORTSTRUCTS_HPP
#define VULKANTIME_GLTFIMPORTSTRUCTS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <stb_image.h>

#include <glm/vec4.hpp>

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

struct Texture {
    // TODO: Fill this out
};

struct Image {
    stbi_uc *data;

    uint32_t width;
    uint32_t height;
    uint32_t size;
};

enum class AlphaMode {
    eOpaque,
    eMask,
    eBlend
};

struct PBRMetallicRoughnessMaterial {
    Texture *baseColorTexture;
    Texture *metallicRoughnessTexture;

    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
};

struct Material {
    std::string name;
    bool hasPBRMetallicRoughness;
    PBRMetallicRoughnessMaterial pbrMetallicRoughness;
};
#endif // VULKANTIME_GLTFIMPORTSTRUCTS_HPP
