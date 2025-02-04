//
// Created by thomas on 30/01/25.
//
#include "descriptorsets.hpp"


// creats a layout for the lighting descriptor set:
lut::DescriptorSetLayout create_lighting_descriptor_layout(
    lut::VulkanWindow const &aWindow) {
    VkDescriptorSetLayoutBinding bindings[2]{};
    // the first binding is the ULighting
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // the second binding is the ULights
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
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

VkDescriptorSet create_lighting_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::Buffer const &aLightingUBO, lut::Buffer const &aLightUBO,
    lut::DescriptorSetLayout const &aLayout) {
    VkDescriptorSet lightingDescriptors =
        lut::alloc_descriptor_set(aWindow, aPool, aLayout.handle);
    // initialize descriptor set with vkUpdateDescriptorSets
    {
        VkWriteDescriptorSet desc[2]{};
        // the first descriptor is the ULighting
        VkDescriptorBufferInfo lightingUboInfo{};
        lightingUboInfo.buffer = aLightingUBO.buffer;
        lightingUboInfo.range = VK_WHOLE_SIZE;

        desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[0].dstSet = lightingDescriptors;
        desc[0].dstBinding = 0;
        desc[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc[0].descriptorCount = 1;
        desc[0].pBufferInfo = &lightingUboInfo;

        // the second descriptor is the ULights
        VkDescriptorBufferInfo lightUboInfo{};
        lightUboInfo.buffer = aLightUBO.buffer;
        lightUboInfo.range = VK_WHOLE_SIZE;

        desc[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[1].dstSet = lightingDescriptors;
        desc[1].dstBinding = 1;
        desc[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc[1].descriptorCount = 1;
        desc[1].pBufferInfo = &lightUboInfo;

        constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
        vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);
    }
    return lightingDescriptors;
}
