//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_MESHMANAGER_HPP
#define VULKANTIME_MESHMANAGER_HPP
#include <vector>

#include "GLTFImportStructs.hpp"
#include "../Context.hpp"
#include "../vkutil/Buffer.hpp"
namespace vk {
class MeshManager {
   public:
    explicit MeshManager(Context &aContext) : mContext(aContext) {};
    ~MeshManager() {
        for (auto &meshGPU : mMeshesGPU) {
            meshGPU.mVertices = {};
            meshGPU.mIndices = {};
        }
    }

    // reserve space for meshes
    void reserveMeshes(size_t aSize, size_t aPrimSize) {
        mMeshes.reserve(aSize);
        mMeshesGPU.resize(aPrimSize);
    }
    // add a mesh to the manager
    void addMesh(Mesh aMesh) {
        mMeshes.push_back(aMesh);
        uploadLastMesh();
    }
    // debug output meshes
    void debugOuptutMeshes();
    // upload the last mesh added to the GPU
    void uploadLastMesh();

    // get a mesh by index
    Mesh *getMesh(size_t aIndex) { return &mMeshes[aIndex]; }

   private:
    std::vector<Mesh> mMeshes;
    std::vector<MeshPrimitiveGPU> mMeshesGPU;
    Context &mContext;
};
}  // namespace vk

#endif  // VULKANTIME_MESHMANAGER_HPP
