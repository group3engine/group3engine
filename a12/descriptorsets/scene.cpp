//
// Created by thomas on 30/01/25.
//

#include "descriptorsets.hpp"
// creates a layout for the scene descriptor set
lut::DescriptorSetLayout create_scene_descriptor_layout(
    lut::VulkanWindow const &aWindow) {
    VkDescriptorSetLayoutBinding bindings[1]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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

// creates a scene descriptor set based on the scene UBO struct
VkDescriptorSet create_scene_descriptor_set(
    lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
    lut::Buffer const &aSceneUBO, lut::DescriptorSetLayout const &aLayout) {
    VkDescriptorSet sceneDescriptors =
        lut::alloc_descriptor_set(aWindow, aPool, aLayout.handle);
    // initialize descriptor set with vkUpdateDescriptorSets
    {
        VkWriteDescriptorSet desc[1]{};

        VkDescriptorBufferInfo sceneUboInfo{};
        sceneUboInfo.buffer = aSceneUBO.buffer;
        sceneUboInfo.range = VK_WHOLE_SIZE;

        desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[0].dstSet = sceneDescriptors;
        desc[0].dstBinding = 0;
        desc[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc[0].descriptorCount = 1;
        desc[0].pBufferInfo = &sceneUboInfo;

        constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
        vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);
    }
    return sceneDescriptors;
}
