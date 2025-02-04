//
// Created by thomas on 30/01/25.
//
#include "descriptorsets.hpp"


// creats a descriptor set layout for the bloom
lut::DescriptorSetLayout create_bloom_descriptor_layout(
    lut::VulkanWindow const &aWindow) {
    VkDescriptorSetLayoutBinding bindings[1]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = bindings;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (auto const res = vkCreateDescriptorSetLayout(
            aWindow.device, &layoutInfo, nullptr, &layout);
        VK_SUCCESS != res) {
        throw lut::Error(
            "Can't create descriptor set layout\n"
            "vkCreateDescriptorSetLayout() returned %s",
            lut::to_string(res).c_str());
    }
    return lut::DescriptorSetLayout(aWindow.device, layout);
}

// creates a descriptor set for the bloom
VkDescriptorSet create_bloom_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::ImageView const &aImageView, lut::Sampler const &aSampler,
    lut::DescriptorSetLayout const &aLayout) {
    VkDescriptorSet set = alloc_descriptor_set(aWindow, aPool, aLayout.handle);

    VkWriteDescriptorSet desc[1]{};
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

    constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
    vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);
    return set;
}