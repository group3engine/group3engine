#pragma once
#include "Volk.hpp"
#include <iostream>
#include <functional>
#include <glm/glm.hpp>
#include <array>
#include <filesystem>

class Context;

    inline std::filesystem::path assetsPath = {};

#define VK_CHECK(call, message)                                                             \
    do {                                                                                    \
        VkResult result = call;                                                             \
        if (result != VK_SUCCESS) {                                                         \
            std::cout << "[VK ERROR]: " << message << " VkResult: " << result << std::endl; \
        }                                                                                   \
    } while (0)

namespace vkutil {

    enum class RenderType {
        FORWARD
    };

    inline size_t MAX_FRAMES_IN_FLIGHT;
    inline int currentFrame;

    inline VkSampler repeatSamplerAniso;
    inline VkSampler repeatSampler;
    inline VkSampler clampToEdgeSamplerAniso;

    inline float maxAnisotropic;
    extern RenderType renderType;
    } // namespace vkutil

    namespace vkutil {
    struct alignas(16) MeshPushConstants {
        glm::mat4 ModelMatrix;
        int cascadeIndex;
    };

    struct LavaPushConstants {
        alignas(16) glm::mat4 ModelMatrix;
        alignas(4) int cascadeIndex;
        alignas(4) float t;
    };

    struct LightUBO {
        alignas(4) int type;
        alignas(16) glm::vec4 LightPosition;
        alignas(16) glm::vec4 LightColour;
        alignas(16) glm::mat4 LightSpaceMatrix;
    };

    struct PostProcessing {
        alignas(4) bool Enable = false;
    };

    struct SSAOSettings
    {
        int NumDirections;
        int NumSteps;
        float Radius;
        float StepSize;
        float intensity;
    };

    struct SSRSettings
    {
        float MaxDistance;
        float thickness;
    };

    struct alignas(16) FogSettings
    {
        float MaxDistance;
        float Density;
        float StepSize;
        int MaxSteps;
    };

    struct FXAASettings
    {
        bool EnableFXAA;
    };

    struct CascadeMatrices
    {
        glm::mat4 matrices[4];
        glm::vec4 cascadeSplits; // Store 4 splits as a vec4
    };

    struct PostProcessingSettings
    {
        float brightness;
        float contrast;
        float saturation;
        int toneMap;
    };

    struct RendererDebug
    {
        int debugMode;
    };

    struct SHCoefficients {
        glm::vec3 SHCoefficients[9];
    };

    inline PostProcessing postProcessSettings = {};
    inline SSAOSettings ssaoSettings = {6, 6, 1.4f, 0.003f, 1.5f};
    inline SSRSettings ssrSettings = {3.0f, 0.001f};
    inline FogSettings fogSettings = { 100.0f, 0.01f, 0.1f, 1 };
    inline FXAASettings fxaaSettings = {true};
    inline PostProcessingSettings postProcessingSettings = {0.0f, 1.0f, 1.0f, 3};
    inline RendererDebug rendererDebug = {0};
    inline uint32_t setRenderingPipeline = 1;
    inline uint32_t setAlphaMakingPipeline = 2;


    inline float ShadowBias = 0.0f;
    inline float ShadowSlope = 3.4f;

    inline VkDescriptorSetLayout materialDescriptorSetLayout;
    inline SHCoefficients SHCoefficientsStored;

    } // namespace vkutil

namespace GlobalUtil {
    inline double deltaTime;
    inline double totalTime;
inline double unscaledDeltaTime;
}

namespace vkutil {
    void ExecuteSingleTimeCommands(Context &context, std::function<void(VkCommandBuffer)> recordCommands);

    // Sync
    void ImageBarrier(
        VkCommandBuffer cmd,
        VkImage img,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkImageLayout srcLayout, VkImageLayout dstLayout,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkImageSubresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

    VkDescriptorSetLayout CreateDescriptorSetLayout(Context &context, const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    void AllocateDescriptorSets(Context &context, VkDescriptorPool descriptorPool, const VkDescriptorSetLayout descriptorLayout, uint32_t setCount, std::vector<VkDescriptorSet> &descriptorSet);
    void AllocateDescriptorSet(Context &context, VkDescriptorPool descriptorPool, const VkDescriptorSetLayout descriptorLayout, uint32_t setCount, VkDescriptorSet &descriptorSet);
    VkDescriptorSetLayoutBinding CreateDescriptorBinding(uint32_t binding, uint32_t count, VkDescriptorType type, VkShaderStageFlags shaderStage);
    void CreateDescriptorPool(Context &context, uint32_t maxDescriptors, uint32_t maxSets, VkDescriptorPool &descriptorPool);
    // Update buffer descriptor
    void UpdateDescriptorSet(Context &context, uint32_t binding, VkDescriptorBufferInfo bufferInfo, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType);
    // Update image descriptor
    void UpdateDescriptorSet(Context &context, uint32_t binding, VkDescriptorImageInfo imageInfo, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType);

    VkSampler CreateSampler(Context &context, VkSamplerAddressMode mode, VkBool32 EnableAnisotropic, VkCompareOp compareOp, VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR, VkSamplerMipmapMode samplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR);

    // Don't use this. Needs to be removed.
    void BulkImageUpdate(Context &context, uint32_t binding, std::vector<VkDescriptorImageInfo> imageInfos, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType);

    inline void RenderPassLabel(VkCommandBuffer commandBuffer, const char *labelName) {
        VkDebugUtilsLabelEXT label = {};
        label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pLabelName = labelName;
        vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
    }

    inline void EndRenderPassLabel(VkCommandBuffer commandBuffer) {
        vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    }
} // namespace vkutil
