//
// Created by thomas on 05/02/25.
//

#ifndef GROUP3ENGINE_RESOURCEMANAGER_HPP
#define GROUP3ENGINE_RESOURCEMANAGER_HPP

#include "GLTFImportStructs.hpp"
#include "MaterialManager.hpp"
#include "MeshManager.hpp"
#include "TextureManager.hpp"
#include <string>
#include <vector>
// predefine meshmanager, texturemanager, materialmanager

#define GLTF_LOAD_FAIL 0
#define GLTF_LOAD_SUCCESS 1

// returns 0 if failed
int LoadGLTF(std::string aFilepath, MeshManager &aMeshManager, MaterialManager &aMaterialManager,
             TextureManager &aTextureManager, bool aIsDebug = false);

#endif // GROUP3ENGINE_RESOURCEMANAGER_HPP
