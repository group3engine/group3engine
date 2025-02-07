//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_GLTFIMPORTSTRUCTS_HPP
#define VULKANTIME_GLTFIMPORTSTRUCTS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <stb_image.h>

#include "../labutils/vkbuffer.hpp"
#include "glm/gtc/quaternion.hpp"
#include <glm/vec4.hpp>

struct MeshPrimitiveGPU {
    labutils::Buffer mPositions;
    labutils::Buffer mTexcoords;
    labutils::Buffer mNormals;
    labutils::Buffer mIndices;
    std::uint32_t mIndexCount;
};
struct MeshPrimitive {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    // TODO: Bone weights
    std::vector<std::uint32_t> indices;
    MeshPrimitiveGPU *meshGPU;

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

struct Transform{
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        // function to get the matrix
        glm::mat4 getMatrix() {
                glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
                glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
                glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
                return translationMatrix * rotationMatrix * scaleMatrix;
        }

};


#endif // VULKANTIME_GLTFIMPORTSTRUCTS_HPP
