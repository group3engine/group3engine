//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_GLTFIMPORTSTRUCTS_HPP
#define VULKANTIME_GLTFIMPORTSTRUCTS_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/gtx/quaternion.hpp>
#include <glm/vec4.hpp>
#include <stb_image.h>

#include "Buffer.hpp"
#include "Image.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/transform.hpp"

/// A vertex, stored on the CPU
struct Vertex {
    /// The position of the vertex
    glm::vec3 pos;
    /// The texture coordinates of the vertex
    glm::vec2 tex;
    /// The normal of the vertex
    glm::vec3 normal;
    /// The joints of the vertex (integer indices)
    glm::vec4 joints;
    /// The weights of the vertex
    glm::vec4 weights;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescrip{};
        bindingDescrip.binding = 0;
        bindingDescrip.stride = sizeof(Vertex);
        bindingDescrip.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescrip;
    }

    static std::array<VkVertexInputAttributeDescription, 5>
    GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 5> attributes = {};

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

        attributes[3].binding = 0;
        attributes[3].location = 3;
        attributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[3].offset = offsetof(Vertex, joints);

        attributes[4].binding = 0;
        attributes[4].location = 4;
        attributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[4].offset = offsetof(Vertex, weights);

        return attributes;
    }
};

// TODO: Fix this problem properly
namespace WPT {
    using Vertex = Vertex;
}

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
          alphaCutout(aOther.alphaCutout), alphaCutoff(aOther.alphaCutoff) {}

    // constructor
    Material() = default;
};

struct MeshPrimitiveGPU {
    Buffer mVertices;
    Buffer mIndices;
    std::uint32_t mIndexCount;
};

/// A primitive, stored as a list of vertices and indices
struct MeshPrimitive {
    /// The vertices that make up the primitive
    std::vector<Vertex> vertices;
    /// The indices that make up the primitive
    std::vector<std::uint32_t> indices;
    MeshPrimitiveGPU *meshGPU;
    /// The material of the primitive
    Material *material;
};

/// A mesh, stored as a list of primitives
struct Mesh {
    /// The name of the mesh
    std::string name;
    /// The primitives that make up the mesh
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
/// A transformation, stored as a translation, rotation, and scale
struct Transform {
    /// The translation of the transform
    glm::vec3 translation;
    /// The rotation of the transform, stored as a quaternion (x, y, z, w)
    glm::quat rotation;
    /// The scale of the transform
    glm::vec3 scale;

    glm::mat4 matrix{};

    /// Update the matrix. If this function is not called since the last change, the matrix will be out of date
    void UpdateMatrix() {
        glm::mat4 translationMatrix = glm::translate(translation);
        glm::mat4 rotationMatrix = glm::toMat4(glm::normalize(rotation));
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        matrix = translationMatrix * rotationMatrix * scaleMatrix;
    }

    /// Get the transform as a glm::mat4
    [[nodiscard]] glm::mat4 getMatrix() const {
        return matrix;
    }

    /// Interpolate between this transform and another. The matrix for the result is updated
    [[nodiscard]] Transform Interpolate(Transform other, float t)
    {
        Transform result{};
        result.translation = translation * (1 - t) + other.translation * t;
        result.rotation = glm::normalize(glm::slerp(rotation, other.rotation, t));
        result.scale = scale * (1-t) + other.scale * t;
        result.UpdateMatrix();
        return result;
    }
};
Transform ZEROTRANSFORM = Transform{glm::vec3(0.0f), glm::quat(0.0f, 0.0f, 0.0f, 1.0f), glm::vec3(1.0f)};
#endif // VULKANTIME_GLTFIMPORTSTRUCTS_HPP
