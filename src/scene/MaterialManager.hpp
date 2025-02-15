#ifndef VULKANTIME_MATERIALMANAGER_HPP
#define VULKANTIME_MATERIALMANAGER_HPP

#include <vector>

#include "../Context.hpp"
#include "../Utils.hpp"
#include "GLTFImportStructs.hpp"
namespace vk {
class MaterialManager {
   public:
    MaterialManager(Context &aContext, VkSampler const &sampler,
                    VkDescriptorSetLayout aMaterialLayout);
    // TODO: this is copying a material by value into the vector, not ideal
    void AddMaterial(Material &material) {
        mMaterials.emplace_back(std::move(material));
        UploadLastMaterial();
    }
    void UploadLastMaterial();
    void ReserveMaterials(size_t size) { mMaterials.reserve(size); }
    Material *GetMaterial(size_t index) { return &mMaterials[index]; }

    void DebugOutputMaterials();

    ~MaterialManager() {
        // destroy the descriptor pool
        vkDestroyDescriptorPool(mContext.device, mDescriptorPool, nullptr);
        // destroy the descriptor set layout
        vkDestroyDescriptorSetLayout(mContext.device, mMaterialLayout, nullptr);
    }

   private:
    VkDescriptorSet create_material_descriptor_set(std::vector<VkImageView> const &aImageViews,
                                                   Buffer const &aMaterialBuffer);
    std::vector<Material> mMaterials;
    Context &mContext;
    VkSampler const &mSampler;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mMaterialLayout;
};
}  // namespace vk
#endif  // VULKANTIME_MATERIALMANAGER_HPP
