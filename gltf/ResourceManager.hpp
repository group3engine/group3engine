//
// Created by thomas on 05/02/25.
//

#ifndef GROUP3ENGINE_RESOURCEMANAGER_HPP
#define GROUP3ENGINE_RESOURCEMANAGER_HPP
#include "MeshManager.hpp"
#include "GLTFImportStructs.hpp"
#include <string>
#include <vector>
// predefine meshmanager, texturemanager, materialmanager
class TextureManager;
class MaterialManager;



#define GLTF_LOAD_FAIL 0
#define GLTF_LOAD_SUCCESS 1

// returns 0 if failed
int LoadGLTF(std::string aFilepath, MeshManager &aMeshManager,
             bool aIsDebug = false);

#endif // GROUP3ENGINE_RESOURCEMANAGER_HPP
