//
// Created by thomas on 30/01/25.
//

#ifndef VULKANTIME_PIPELINE_H
#define VULKANTIME_PIPELINE_H

#include "vkobject.hpp"
#include "vulkan_context.hpp"
#include "vulkan_window.hpp"
#include "error.hpp"
#include "to_string.hpp"
#include "vkutil.hpp"
#include <glm/glm.hpp>

namespace lut = labutils;

lut::PipelineLayout create_basic_pipeline_layout(
    lut::VulkanContext const &, VkDescriptorSetLayout *aLayouts,
    size_t aNumLayouts);

lut::Pipeline create_basic_pipeline(lut::VulkanWindow const &, VkRenderPass,
                                    VkPipelineLayout, char const *,
                                    char const *,
                                    size_t numColourAttachments = 1,
                                    char const *aDebugName = "");

lut::Pipeline create_alpha_pipeline(
    lut::VulkanWindow const &, VkRenderPass, VkPipelineLayout, char const *,
    char const *, VkBlendFactor aSrcBlend = VK_BLEND_FACTOR_SRC_ALPHA,
    VkBlendFactor aDstBlend =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, /* cull mode */
    VkCullModeFlagBits aCullMode = VK_CULL_MODE_NONE,
    VkBool32 aEnableDepthTest = VK_TRUE);

lut::PipelineLayout create_postprocess_pipeline_layout(
    lut::VulkanContext const &aContext, VkDescriptorSetLayout *aLayouts,
    size_t aNumLayouts);
lut::Pipeline create_postprocess_pipeline(lut::VulkanWindow const &,
                                          VkRenderPass, VkPipelineLayout,
                                          char const *, char const *,
                                          bool aEnableDepthTest = false);
#endif  // VULKANTIME_PIPELINE_H
