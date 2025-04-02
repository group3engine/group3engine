//
// Created by thomas on 05/02/25.
//

#ifndef GROUP3ENGINE_RESOURCEMANAGER_HPP
#define GROUP3ENGINE_RESOURCEMANAGER_HPP

#include "Entity.hpp"
#include "GLTFImportStructs.hpp"
#include "MaterialManager.hpp"
#include "MeshManager.hpp"
#include "TextureManager.hpp"
#include <string>
#include <vector>
// predefine meshmanager, texturemanager, materialmanager

#define GLTF_LOAD_FAIL 0
#define GLTF_LOAD_SUCCESS 1

namespace ResourceLoader {

// returns 0 if failed
int LoadGLTF(std::filesystem::path aFilepath, MeshManager &aMeshManager,
             MaterialManager &aMaterialManager, TextureManager &aTextureManager,
             std::vector<Entity *> &aEntities, bool aIsDebug,
             std::vector<Animation> &aAnimations, std::vector<Skin> &aSkins,
             std::vector<Entity *> &aCharacterEntities);

} // namespace ResourceLoader
#endif // GROUP3ENGINE_RESOURCEMANAGER_HPP
