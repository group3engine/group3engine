#include "descriptorsets.hpp"

lut::DescriptorSetLayout create_material_descriptor_layout(lut::VulkanWindow const &aWindow) {
    // base colour, roughness, metalness, alphamask, normalmap, emissive
    VkDescriptorSetLayoutBinding bindings[6]{};
    // base colour
    bindings[0].binding = 0; // this must match the binding in the shader
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // roughness
    bindings[1].binding = 1; // this must match the binding in the shader
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // metalness
    bindings[2].binding = 2; // this must match the binding in the shader
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // alphamask
    bindings[3].binding = 3; // this must match the binding in the shader
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // normalmap
    bindings[4].binding = 4; // this must match the binding in the shader
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // emissive
    bindings[5].binding = 5; // this must match the binding in the shader
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layoutInfo.pBindings = bindings;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout);
        VK_SUCCESS != res) {
        throw lut::Error("Can't create descriptor set layout\n"
                    "vkCreateDescriptorSetLayout() returned %s",
                    lut::to_string(res).c_str());
    }

    return lut::DescriptorSetLayout(aWindow.device, layout);
}

VkDescriptorSet create_material_descriptor_set(lut::VulkanWindow const &aWindow,
                                               lut::DescriptorPool const &aPool,
                                               BakedMaterialInfo const &aMaterialInfo,
                                               std::vector<lut::ImageView> const &aImageViews,
                                               lut::Sampler const &aSampler,
                                               lut::DescriptorSetLayout const &aMaterialLayout) {
    // allocate the descriptor set for this material
    VkDescriptorSet set = alloc_descriptor_set(aWindow, aPool, aMaterialLayout.handle);
    // write the descriptor set
    VkWriteDescriptorSet desc[6]{};
    VkDescriptorImageInfo textureInfo[6]{};
    // write the texture infos
    for (int i = 0; i < 6; i++) {
        textureInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // if the material index is out of bounds, use the base colour (0 index)
        if (aMaterialInfo[i] < aImageViews.size()) {
            textureInfo[i].imageView = aImageViews[aMaterialInfo[i]].handle;
            textureInfo[i].sampler = aSampler.handle;
        } else {
            textureInfo[i].imageView = aImageViews[aMaterialInfo.baseColorTextureId].handle;
            textureInfo[i].sampler = aSampler.handle;
        }
    }
    // write the descriptor sets
    for (int i = 0; i < 6; i++) {
        desc[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[i].dstSet = set;
        desc[i].dstBinding = i;
        desc[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[i].descriptorCount = 1;
        desc[i].pImageInfo = &textureInfo[i];
    }
    // update the descriptor sets
    constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
    vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);

    return set;
}
