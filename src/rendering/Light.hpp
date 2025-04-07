#pragma once

#include <glm/glm.hpp>
#include "Utils.hpp"

constexpr int NUM_POINT_LIGHTS = 25;
constexpr int NUM_DIRECTIONAL_LIGHTS = 1;
constexpr int NUM_LIGHTS = NUM_POINT_LIGHTS + NUM_DIRECTIONAL_LIGHTS;

struct LightBuffer {
    vkutil::LightUBO lights[NUM_LIGHTS];
};

/// The light type enum is used to define the type of light
enum class LightType { Directional, Point };

/// The light struct is used to store the light data
struct Light {
    /// The light type
    LightType Type = LightType::Directional;
    glm::vec4 position;
    glm::vec4 colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 LightSpaceMatrix = glm::mat4(1.0f);
    float view = -10.0f;
    float near = 0.1f;
    float far = 55.0f;
};
