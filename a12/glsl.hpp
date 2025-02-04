//
// Created by thomas on 28/12/24.
//

#ifndef VULKANTIME_GLSL_HPP
#define VULKANTIME_GLSL_HPP

#include "../labutils/vkbuffer.hpp"
#include "../labutils/vkimage.hpp"
#include "glm/detail/type_mat4x4.hpp"

namespace glsl {

#define MAX_LIGHTS 32
#define MAX_SHADOW_LIGHTS 4
#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_DIRECTIONAL_SHADOW_LIGHTS 4
constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

namespace lut = labutils;

struct SceneUniform {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
    glm::vec4 cameraPosition;
};

struct LightingUniform {
    glm::vec3 ambientLight;
    int numLights;
    int numShadowLights;
    int numDirectionalLights;
    int numDirectionalShadowLights;
};

struct PointLightShadowed {
    glm::vec4 position;
    glm::vec4 color;
    glm::mat4 shadowProj;
};
struct PointLight {
    glm::vec4 position;
    glm::vec4 color;
};
struct DirectionalLight {
    glm::vec4 direction;
    glm::vec4 color;
};
struct DirectionalLightShadowed {
    glm::vec4 direction;
    glm::vec4    color;
    glm::mat4 shadowProj;
};

struct LightUniform {
    PointLight lights[MAX_LIGHTS];
    PointLightShadowed shadowLights[MAX_SHADOW_LIGHTS];
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    DirectionalLightShadowed directionalShadowLights[MAX_DIRECTIONAL_SHADOW_LIGHTS];
};

static_assert(sizeof(SceneUniform) <= 65536,
              "Uniform buffer size must be less than 64KB");
static_assert(sizeof(SceneUniform) % 4 == 0,
              "Uniform buffer size must be a multiple of 4 bytes");

static_assert(sizeof(LightingUniform) <= 65536,
              "Uniform buffer size must be less than 64KB");
static_assert(sizeof(LightingUniform) % 4 == 0,
              "Uniform buffer size must be a multiple of 4 bytes");

static_assert(sizeof(LightUniform) <= 65536,
              "Uniform buffer size must be less than 64KB");
static_assert(sizeof(LightUniform) % 4 == 0,
              "Uniform buffer size must be a multiple of 4 bytes");

struct UBO {
    void *uniformData;
    std::size_t uniformSize;
    VkBuffer uniformBuffer;
    VkPipelineStageFlags externalStageFlags;
};

struct GBuffer {
    // images and image views for the GBuffer
    std::vector<lut::Image> depthImages;
    std::vector<lut::ImageView> depthImageViews;
    std::vector<lut::Image> normalImages;
    std::vector<lut::ImageView> normalImageViews;
    std::vector<lut::Image> albedoMetallicImages;
    std::vector<lut::ImageView> albedoMetallicImageViews;
    std::vector<lut::Image> emmisiveRoughnessImages;
    std::vector<lut::ImageView> emmisiveRoughnessImageViews;
};

}  // namespace glsl
#endif  // VULKANTIME_GLSL_HPP
