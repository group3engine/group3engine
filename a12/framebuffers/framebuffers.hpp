//
// Created by thomas on 30/01/25.
//

#ifndef VULKANTIME_FRAMEBUFFERS_HPP
#define VULKANTIME_FRAMEBUFFERS_HPP

#include <tuple>

#include "../../labutils/vkimage.hpp"
#include "../../labutils/vkobject.hpp"
#include "../../labutils/vulkan_window.hpp"
#include "../../labutils/error.hpp"
#include "../../labutils/to_string.hpp"
#include "../../labutils/vkimage.hpp"
#include "../../labutils/vkutil.hpp"
namespace lut = labutils;


lut::Framebuffer create_framebuffer(
    lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass,
    std::vector<VkImageView> aAttachments C5_DBGNAME_DECL());

void create_swapchain_framebuffers(
    lut::VulkanWindow &aWindow, VkRenderPass aRenderPass);

std::tuple<lut::Image, lut::ImageView> create_render_texture(
    lut::VulkanWindow const &aWindow, lut::Allocator const &aAllocator);

std::tuple<lut::Image, lut::ImageView> create_depth_buffer(
    lut::VulkanWindow const &aWindow, lut::Allocator const &aAllocator, VkFormat aDepthFormat);

#endif  // VULKANTIME_FRAMEBUFFERS_HPP
