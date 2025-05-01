//
// Created by thomas on 01/04/25.
//

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "LightManager.hpp"
#include "Utils.hpp"

namespace {
    template<std::size_t N>
    size_t FirstAvailableLightIndex(const std::bitset<N> &lightMask) {
        size_t i = 0;
        for (; i < N; ++i) {
            if (!lightMask[i]) {
                return i;
            }
        }

        return i;
    }
}

std::tuple<Light *, int> LightManager::GetDirectionalLight()
{
    // find the first available light in the mask
    size_t lightIndex = FirstAvailableLightIndex(mDirectionalLightMask);
    if (lightIndex == NUM_DIRECTIONAL_LIGHTS) {
        return {nullptr, -1};
    }
    mDirectionalLightMask[lightIndex] = true;
    return {&mDirectionalLights[lightIndex], lightIndex};
}

std::tuple<Light *, int> LightManager::GetPointLight()
{
    // find the first available light in the mask
    size_t lightIndex = FirstAvailableLightIndex(mPointLightMask);
    if (lightIndex == NUM_POINT_LIGHTS) {
        return {nullptr, -1};
    }
    mPointLightMask[lightIndex] = true;
    return {&mPointLights[lightIndex], lightIndex};
}

void LightManager::ReturnDirectionalLight(int index)
{
    mDirectionalLightMask[index] = false;
}

void LightManager::ReturnPointLight(int index)
{
    mPointLightMask[index] = false;
}

void LightManager::Update()
{
    // for each directional light, update the lightspace matrix
    for (size_t i = 0; i < NUM_DIRECTIONAL_LIGHTS; i++) {
        if (mDirectionalLightMask[i]) {
            mDirectionalLights[i].LightSpaceMatrix = glm::ortho(-mDirectionalLights[i].view, mDirectionalLights[i].view, -mDirectionalLights[i].view, mDirectionalLights[i].view, mDirectionalLights[i].near_, mDirectionalLights[i].far_) *
                                                     glm::lookAt(glm::vec3(mDirectionalLights[i].position), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0));
        }
    }
    size_t lightCount = 0;
    // for each directional light
    for (size_t i = 0; i < NUM_DIRECTIONAL_LIGHTS; i++) {
        if (mDirectionalLightMask[i]) {
            m_LightBuffer.lights[lightCount].type = static_cast<int>(mDirectionalLights[i].Type);
            m_LightBuffer.lights[lightCount].LightPosition = mDirectionalLights[i].position;
            m_LightBuffer.lights[lightCount].LightColour = mDirectionalLights[i].colour;
            m_LightBuffer.lights[lightCount].LightSpaceMatrix = mDirectionalLights[i].LightSpaceMatrix;
            lightCount++;
        }
        else
        {
            // set the colour to 0
            m_LightBuffer.lights[lightCount].LightColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    // for each point light
    for (size_t i = 0; i < NUM_POINT_LIGHTS; i++) {
        if (mPointLightMask[i]) {
            m_LightBuffer.lights[lightCount].type = static_cast<int>(mPointLights[i].Type);
            m_LightBuffer.lights[lightCount].LightPosition = mPointLights[i].position;
            m_LightBuffer.lights[lightCount].LightColour = mPointLights[i].colour;
            m_LightBuffer.lights[lightCount].LightSpaceMatrix = mPointLights[i].LightSpaceMatrix;
            lightCount++;
        }
        else
        {
            // set the colour to 0
            m_LightBuffer.lights[lightCount].LightColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

}

void LightManager::UploadLights(VkCommandBuffer cmdBuff)
{
    m_LightUBO[vkutil::currentFrame].Upload(cmdBuff, &m_LightBuffer, sizeof(LightBuffer));
}

void LightManager::Unload() {
    mDirectionalLightMask.reset();
    mPointLightMask.reset();
}

void LightManager::Destroy()
{
    for (auto &buffer : m_LightUBO) {
        buffer.Destroy();
    }
    m_LightUBO.clear();
    mDirectionalLightMask.reset();
    mPointLightMask.reset();
    m_LightBuffer = {};
}

void LightManager::StartUp(Context &aContext)
{
    m_LightUBO.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    for (auto &buffer : m_LightUBO) {
        buffer = CreateBuffer("LightUBO", aContext, sizeof(LightBuffer),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                              VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                              VMA_ALLOCATION_CREATE_MAPPED_BIT);
    }
    // clear the bitset
    mDirectionalLightMask.reset();
    mPointLightMask.reset();

}

std::vector<Light*> LightManager::GetLights()
{
    // construct a vector of lights
    std::vector<Light*> lights;
    // for each directional light
    for (size_t i = 0; i < NUM_DIRECTIONAL_LIGHTS; i++) {
        if (mDirectionalLightMask[i]) {
            lights.push_back(&mDirectionalLights[i]);
        }
    }
    // for each point light
    for (size_t i = 0; i < NUM_POINT_LIGHTS; i++) {
        if (mPointLightMask[i]) {
            lights.push_back(&mPointLights[i]);
        }
    }
    return lights;
}

std::tuple<Light *, int> LightManager::SetDirectionalLight(Light *light)
{
    assert(light->Type == LightType::Directional);
    // get the first available light
    Light* lightPtr;
    int lightIndex;
    std::tie(lightPtr, lightIndex) = GetDirectionalLight();
    if (lightPtr == nullptr) {
        return {nullptr, -1};
    }
    // set the light data
    *lightPtr = *light;
    return {lightPtr, lightIndex};
}

std::tuple<Light *, int> LightManager::SetPointLight(Light *light)
{
    assert(light->Type == LightType::Point);
    // get the first available light
    Light* lightPtr;
    int lightIndex;
    std::tie(lightPtr, lightIndex) = GetPointLight();
    if (lightPtr == nullptr) {
        return {nullptr, -1};
    }
    // set the light data
    *lightPtr = *light;
    return {lightPtr, lightIndex};
}



