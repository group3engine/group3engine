//
// Created by thomas on 30/01/25.
//
#include "descriptorsets.hpp"

// creates a descriptor set for the post process pipeline
VkDescriptorSet create_post_process_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::ImageView const &aImageView, lut::ImageView const &aDepthView,
    lut::Sampler const &aSampler, lut::DescriptorSetLayout const &aLayout) {
    VkDescriptorSet set = alloc_descriptor_set(aWindow, aPool, aLayout.handle);

    VkWriteDescriptorSet desc[2]{};
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = aImageView.handle;
    imageInfo.sampler = aSampler.handle;

    desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc[0].dstSet = set;
    desc[0].dstBinding = 0;
    desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc[0].descriptorCount = 1;
    desc[0].pImageInfo = &imageInfo;

    VkDescriptorImageInfo depthInfo{};
    depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    depthInfo.imageView = aDepthView.handle;
    depthInfo.sampler = aSampler.handle;

    desc[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc[1].dstSet = set;
    desc[1].dstBinding = 1;
    desc[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc[1].descriptorCount = 1;
    desc[1].pImageInfo = &depthInfo;

    constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
    vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);
    return set;
}
