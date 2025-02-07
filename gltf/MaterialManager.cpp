#include "MaterialManager.hpp"

#include <iostream>

#include <glm/ext.hpp>

void MaterialManager::DebugOutputMaterials() {
    std::cout << "materials.size()=" << mMaterials.size() << '\n';

    const auto &material = mMaterials[1];
    std::cout << "material.name=" << material.name << '\n';
    std::cout << "material.hasPBRMetallicRoughness=" << material.hasPBRMetallicRoughness << '\n';
    std::cout << "material.pbrMetallicRoughness.baseColorFactor=" << glm::to_string(material.pbrMetallicRoughness.baseColorFactor) << '\n';
    std::cout << "material.pbrMetallicRoughness.metallicFactor=" << material.pbrMetallicRoughness.metallicFactor << '\n';
    std::cout << "material.pbrMetallicRoughness.roughnessFactor=" << material.pbrMetallicRoughness.roughnessFactor << '\n';
}
