#ifndef VULKANTIME_MATERIALMANAGER_HPP
#define VULKANTIME_MATERIALMANAGER_HPP

#include <vector>

#include "Context.hpp"
#include "Utils.hpp"
#include "GLTFImportStructs.hpp"

class MaterialManager {
  public:
    MaterialManager(Context &aContext);

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
        vkDestroyDescriptorSetLayout(mContext.device, vkutil::materialDescriptorSetLayout, nullptr);

        for (auto &material : mMaterials) {
            material.materialBuffer.Destroy();
        }
    }

  private:
    VkDescriptorSet create_material_descriptor_set(std::vector<VkImageView> const &aImageViews,
                                                   Buffer const &aMaterialBuffer);

    VkDescriptorSetLayout create_material_descriptor_layout() const;
    std::vector<Material> mMaterials;
    Context &mContext;
    VkDescriptorPool mDescriptorPool;
};
#endif // VULKANTIME_MATERIALMANAGER_HPP
