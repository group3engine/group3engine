//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_GLTFIMPORTSTRUCTS_HPP
#define VULKANTIME_GLTFIMPORTSTRUCTS_HPP

#include <stb_image.h>

#include <cstdint>
#include <glm/vec4.hpp>
#include <string>
#include <vector>

#include "Buffer.hpp"
#include "Image.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/transform.hpp"

namespace vk {

struct Vertex {
    glm::vec3 pos;
    glm::vec2 tex;
    glm::vec3 normal;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescrip{};
        bindingDescrip.binding = 0;
        bindingDescrip.stride = sizeof(Vertex);
        bindingDescrip.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescrip;
    }

    static std::array<VkVertexInputAttributeDescription, 3>
    GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributes = {};

        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[0].offset = offsetof(Vertex, pos);

        attributes[1].binding = 0;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[1].offset = offsetof(Vertex, tex);

        attributes[2].binding = 0;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[2].offset = offsetof(Vertex, normal);

        return attributes;
    }
};

struct Texture {
    Image image;
    std::string name;
};
struct PBRMaterialNumbers {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
};

struct PBRMetallicRoughnessMaterial {
    Texture *baseColorTexture;
    Texture *metallicRoughnessTexture;
    PBRMaterialNumbers pbrMaterialNumbers;
    std::string baseColorTextureName;

    std::string metallicRoughnessTextureName;
};

struct Material {
    std::string name;
    bool hasPBRMetallicRoughness;
    PBRMetallicRoughnessMaterial pbrMetallicRoughness;
    VkDescriptorSet descriptorSet;
    Buffer materialBuffer;
    bool alphaCutout;
    float alphaCutoff;
    // move constructor
    Material(Material &&aOther) noexcept
        : name(std::move(aOther.name)),
          hasPBRMetallicRoughness(aOther.hasPBRMetallicRoughness),
          pbrMetallicRoughness(aOther.pbrMetallicRoughness),
          descriptorSet(aOther.descriptorSet),
          materialBuffer(std::move(aOther.materialBuffer)),
          alphaCutout(aOther.alphaCutout),
          alphaCutoff(aOther.alphaCutoff) {}

    // constructor
    Material() = default;
};

struct MeshPrimitiveGPU {
    Buffer mPositions;
    Buffer mTexcoords;
    Buffer mNormals;
    Buffer mIndices;
    std::uint32_t mIndexCount;
};
struct MeshPrimitive {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    // TODO: Bone weights
    std::vector<std::uint32_t> indices;
    MeshPrimitiveGPU *meshGPU;
    Material *material;
};

struct Mesh {
    std::string name;
    std::vector<MeshPrimitive> meshPrimitives;
};

struct Image_STB {
    std::string name;

    stbi_uc *data;

    uint32_t width;
    uint32_t height;
    uint32_t size;
};

enum class AlphaMode { eOpaque, eMask, eBlend };

struct Transform {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    // function to get the matrix
    [[nodiscard]] glm::mat4 getMatrix() const {
        glm::mat4 translationMatrix = glm::translate(translation);
        glm::mat4 rotationMatrix = glm::mat4_cast(glm::normalize(rotation));
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        return translationMatrix * rotationMatrix * scaleMatrix;
    }
};
}  // namespace vk

#endif  // VULKANTIME_GLTFIMPORTSTRUCTS_HPP
