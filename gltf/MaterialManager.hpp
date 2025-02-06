#ifndef VULKANTIME_MATERIALMANAGER_HPP
#define VULKANTIME_MATERIALMANAGER_HPP

#include "GLTFImportStructs.hpp"
#include <vector>

class MaterialManager {
  public:
    // TODO: this is copying a material by value into the vector, not ideal
    void AddMaterial(Material material) { mMaterials.push_back(material); }

    void ReserveMaterials(size_t size) {
      mMaterials.reserve(size);
    }

    void DebugOutputMeshes();

  private:
    std::vector<Material> mMaterials;
};
#endif // VULKANTIME_MATERIALMANAGER_HPP
