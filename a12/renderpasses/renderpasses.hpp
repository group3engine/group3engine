//
// Created by thomas on 30/01/25.
//

#ifndef VULKANTIME_RENDERPASSES_HPP
#define VULKANTIME_RENDERPASSES_HPP
#include "error.hpp"
#include "to_string.hpp"
#include "vkobject.hpp"
#include "vulkan_window.hpp"
namespace lut = labutils;
lut::RenderPass create_render_pass_to_texture(
    lut::VulkanWindow const &aWindow, VkFormat aDepthFormat C5_DBGNAME_DECL());

lut::RenderPass create_postprocess_render_pass(
    lut::VulkanWindow const &, bool, VkFormat,
    bool hasDepth = false C5_DBGNAME_DECL());
#endif  // VULKANTIME_RENDERPASSES_HPP
