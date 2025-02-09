//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_MESHMANAGER_HPP
#define VULKANTIME_MESHMANAGER_HPP
#include "allocator.hpp"
#include "vkbuffer.hpp"
#include "vulkan_context.hpp"
#include "GLTFImportStructs.hpp"
#include <vector>
namespace lut = labutils;



class MeshManager {
  public:
    MeshManager(lut::VulkanContext const &aContext, lut::Allocator const &aAllocator)
        : mContext(aContext), mAllocator(aAllocator) {};
    ~MeshManager(){
        for(auto &meshGPU : mMeshesGPU) {
            meshGPU.mPositions = {};
            meshGPU.mTexcoords = {};
            meshGPU.mNormals = {};
            meshGPU.mIndices = {};
        }
    }

    // reserve space for meshes
    void reserveMeshes(size_t aSize, size_t aPrimSize) { mMeshes.reserve(aSize); mMeshesGPU.resize(aPrimSize); }
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
    lut::VulkanContext const &mContext;
    lut::Allocator const &mAllocator;
};

#endif // VULKANTIME_MESHMANAGER_HPP
