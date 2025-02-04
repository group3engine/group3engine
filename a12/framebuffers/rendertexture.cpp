//
// Created by thomas on 30/01/25.
//
#include "framebuffers.hpp"

// create a render texture image and view
std::tuple<lut::Image, lut::ImageView> create_render_texture(
    lut::VulkanWindow const &aWindow, lut::Allocator const &aAllocator) {
    lut::Image renderTexture = lut::create_image_texture2d(
        aAllocator, aWindow.swapchainExtent.width,
        aWindow.swapchainExtent.height, aWindow.swapchainFormat,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        aWindow.device, 1);

    // generate a render texture view
    lut::ImageView renderTextureView = lut::create_image_view_texture2d(
        aWindow, renderTexture.image, aWindow.swapchainFormat);

    return {std::move(renderTexture), std::move(renderTextureView)};
}

// create a depth buffer image and view
std::tuple<lut::Image, lut::ImageView> create_depth_buffer(
    lut::VulkanWindow const &aWindow, lut::Allocator const &aAllocator, VkFormat aDepthFormat) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = aDepthFormat;
    imageInfo.extent.width = aWindow.swapchainExtent.width;
    imageInfo.extent.height = aWindow.swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.mipLevels = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    if (auto const res =
            vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image,
                           &allocation, nullptr);
        VK_SUCCESS != res) {
        throw lut::Error(
            "Can't create depth buffer image\n"
            "vmaCreateImage() returned %s",
            lut::to_string(res).c_str());
    }

    lut::Image depthImage(aAllocator.allocator, image, allocation);

    // create the image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = aDepthFormat;
    viewInfo.components = VkComponentMapping{};
    viewInfo.subresourceRange =
        VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VkImageView view = VK_NULL_HANDLE;
    if (auto const res =
            vkCreateImageView(aWindow.device, &viewInfo, nullptr, &view);
        VK_SUCCESS != res) {
        throw lut::Error(
            "Can't create depth buffer image view\n"
            "vkCreateImageView() returned %s",
            lut::to_string(res).c_str());
    }

    return {std::move(depthImage), lut::ImageView{aWindow.device, view}};
}

