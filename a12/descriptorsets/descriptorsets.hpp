//
// Created by thomas on 30/01/25.
//

#ifndef VULKANTIME_DESCRIPTORSETS_HPP
#define VULKANTIME_DESCRIPTORSETS_HPP

#include "../baked_model.hpp"

#include "../../labutils/error.hpp"
#include "../../labutils/to_string.hpp"
#include "../../labutils/vkbuffer.hpp"
#include "../../labutils/vkobject.hpp"
#include "../../labutils/vkutil.hpp"
#include "../../labutils/vulkan_window.hpp"
#include "vulkan/vulkan.h"
namespace lut = labutils;

lut::DescriptorSetLayout create_scene_descriptor_layout(
    lut::VulkanWindow const &);
VkDescriptorSet create_scene_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::Buffer const &aSceneUBO, lut::DescriptorSetLayout const &aLayout);

VkDescriptorSet create_post_process_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::ImageView const &aImageView, lut::ImageView const &aDepthView,
    lut::Sampler const &aSampler, lut::DescriptorSetLayout const &aLayout);

lut::DescriptorSetLayout create_bloom_descriptor_layout(
    lut::VulkanWindow const &aWindow);
VkDescriptorSet create_bloom_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::ImageView const &aImageView, lut::Sampler const &aSampler,
    lut::DescriptorSetLayout const &aLayout);

lut::DescriptorSetLayout create_lighting_descriptor_layout(
    lut::VulkanWindow const &);
VkDescriptorSet create_lighting_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::Buffer const &aLightingUBO, lut::Buffer const &aLightUBO,
    lut::DescriptorSetLayout const &aLayout);

lut::DescriptorSetLayout create_material_descriptor_layout(lut::VulkanWindow const &);

VkDescriptorSet create_material_descriptor_set(lut::VulkanWindow const &aWindow,
                                               lut::DescriptorPool const &aPool,
                                               BakedMaterialInfo const &aMaterialInfo,
                                               std::vector<lut::ImageView> const &aImageViews,
                                               lut::Sampler const &aSampler,
                                               lut::DescriptorSetLayout const &aMaterialLayout);
#endif  // VULKANTIME_DESCRIPTORSETS_HPP
