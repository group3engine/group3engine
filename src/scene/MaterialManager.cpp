#include "MaterialManager.hpp"

#include <glm/ext.hpp>
#include <iostream>

namespace vk {

void MaterialManager::DebugOutputMaterials() {
    for (auto &material : mMaterials) {
        // assert that the texture name is the same as the one in the material
        assert(material.pbrMetallicRoughness.baseColorTextureName ==
               material.pbrMetallicRoughness.baseColorTexture->name);
        assert(material.pbrMetallicRoughness.metallicRoughnessTextureName ==
               material.pbrMetallicRoughness.metallicRoughnessTexture->name);
    }
    std::cout << "materials.size()=" << mMaterials.size() << '\n';

    const auto &material = mMaterials[1];
    std::cout << "material.name=" << material.name << '\n';
    std::cout << "material.hasPBRMetallicRoughness="
              << material.hasPBRMetallicRoughness << '\n';
    std::cout << "material.pbrMetallicRoughness.baseColorFactor="
              << glm::to_string(material.pbrMetallicRoughness.pbrMaterialNumbers
                                    .baseColorFactor)
              << '\n';
    std::cout << "material.pbrMetallicRoughness.metallicFactor="
              << material.pbrMetallicRoughness.pbrMaterialNumbers.metallicFactor
              << '\n';
    std::cout
        << "material.pbrMetallicRoughness.roughnessFactor="
        << material.pbrMetallicRoughness.pbrMaterialNumbers.roughnessFactor
        << '\n';
}
void MaterialManager::UploadLastMaterial() {
    auto &material = mMaterials.back();
    // create the material buffer
    vk::CreateAndUploadBuffer(
        mContext, &material.pbrMetallicRoughness.pbrMaterialNumbers,
        sizeof(PBRMaterialNumbers),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        material.materialBuffer);

    // now create the descriptor set
    std::vector<VkImageView> imageViews;
    imageViews.emplace_back(
        material.pbrMetallicRoughness.baseColorTexture->image.imageView);
    imageViews.emplace_back(material.pbrMetallicRoughness
                                .metallicRoughnessTexture->image.imageView);
    material.descriptorSet =
        create_material_descriptor_set(imageViews, material.materialBuffer);
}
MaterialManager::MaterialManager(Context &aContext, VkSampler const &sampler,
                                 VkDescriptorSetLayout aMaterialLayout)
    : mContext(aContext), mSampler(sampler), mMaterialLayout(aMaterialLayout) {
    // create the descriptor pool
    mDescriptorPool = VK_NULL_HANDLE;
    vk::CreateDescriptorPool(mContext, 2048, 1024, mDescriptorPool);
}

VkDescriptorSet MaterialManager::create_material_descriptor_set(
    std::vector<VkImageView> const &aImageViews,
    Buffer const &aMaterialBuffer) {
    // allocate the descriptor set for this material
    VkDescriptorSet set = VK_NULL_HANDLE;
    vk::AllocateDescriptorSet(mContext, mDescriptorPool, mMaterialLayout, 1,
                              set);
    // write the descriptor set
    VkWriteDescriptorSet desc[3]{};
    VkDescriptorImageInfo textureInfo[3]{};
    // write the texture infos
    for (int i = 0; i < 2; i++) {
        textureInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if (i < static_cast<int>(aImageViews.size()) &&
            aImageViews[i] != VK_NULL_HANDLE) {
            textureInfo[i].imageView = aImageViews[i];
        } else {
            textureInfo[i].imageView = aImageViews[0];
        }
        textureInfo[i].sampler = mSampler;
    }
    // write the descriptor sets
    for (int i = 0; i < 2; i++) {
        desc[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[i].dstSet = set;
        desc[i].dstBinding = i;
        desc[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[i].descriptorCount = 1;
        desc[i].pImageInfo = &textureInfo[i];
    }
    // write the material numbers
    // create the pbufferinfo
    VkDescriptorBufferInfo aMaterialBufferInfo{};
    aMaterialBufferInfo.buffer = aMaterialBuffer.buffer;
    aMaterialBufferInfo.offset = 0;
    aMaterialBufferInfo.range = VK_WHOLE_SIZE;

    desc[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc[2].dstSet = set;
    desc[2].dstBinding = 2;
    desc[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc[2].descriptorCount = 1;
    desc[2].pBufferInfo = &aMaterialBufferInfo;
    // update the descriptor sets
    constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
    vkUpdateDescriptorSets(mContext.device, numSets, desc, 0, nullptr);

    return set;
}
}  // namespace vk