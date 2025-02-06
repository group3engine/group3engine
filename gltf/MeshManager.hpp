//
// Created by thomas on 05/02/25.
//

#ifndef VULKANTIME_MESHMANAGER_HPP
#define VULKANTIME_MESHMANAGER_HPP
#include "../labutils/allocator.hpp"
#include "../labutils/vkbuffer.hpp"
#include "../labutils/vulkan_context.hpp"
#include "GLTFImportStructs.hpp"
#include <vector>
namespace lut = labutils;

struct MeshPrimitiveGPU {
    labutils::Buffer mPositions;
    labutils::Buffer mTexcoords;
    labutils::Buffer mNormals;
    labutils::Buffer mIndices;
    std::uint32_t mIndexCount;
};

class MeshManager {
  public:
    MeshManager(lut::VulkanContext const &aContext, lut::Allocator const &aAllocator,
                VkPipelineLayout pipelineLayout)
        : mContext(aContext), mAllocator(aAllocator), mPipelineLayout(pipelineLayout) {};
    ~MeshManager(){
        for(auto &meshGPU : mMeshesGPU) {
            meshGPU.mPositions = {};
            meshGPU.mTexcoords = {};
            meshGPU.mNormals = {};
            meshGPU.mIndices = {};
        }
    }

    // reserve space for meshes
    void reserveMeshes(size_t aSize) { mMeshes.reserve(aSize); }
    // add a mesh to the manager
    void addMesh(Mesh aMesh) {
        mMeshes.push_back(aMesh);
        uploadLastMesh();
    }
    // debug output meshes
    void debugOuptutMeshes();
    // upload the last mesh added to the GPU
    void uploadLastMesh();

    // TMP
    void record_draw(VkCommandBuffer aCmdBuff) const;
    void record_draw_shadow(VkCommandBuffer aCmdBuff) const;

  private:
    std::vector<Mesh> mMeshes;
    std::vector<MeshPrimitiveGPU> mMeshesGPU;
    lut::VulkanContext const &mContext;
    lut::Allocator const &mAllocator;
    VkPipelineLayout mPipelineLayout;
};

#endif // VULKANTIME_MESHMANAGER_HPP
