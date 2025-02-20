#ifndef SCENE_MESHMANAGER_HPP
#define SCENE_MESHMANAGER_HPP

#include <vector>

#include "Context.hpp"
#include "Buffer.hpp"
#include "GLTFImportStructs.hpp"

class MeshManager {
  public:
    explicit MeshManager(Context &aContext)
        : mContext(aContext){};

    ~MeshManager() {
        for (auto &meshGPU : mMeshesGPU) {
            meshGPU.mVertices.Destroy();
            meshGPU.mIndices.Destroy();
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
#endif // VULKANTIME_MESHMANAGER_HPP
