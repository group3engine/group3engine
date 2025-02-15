//
// Created by thomas on 05/02/25.
//

#include "MeshManager.hpp"

#include <cstring>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
namespace vk {
void MeshManager::debugOuptutMeshes() {
    std::cout << "meshes.size()=" << mMeshes.size() << '\n';
    std::cout << "meshPrimitives.size()=" << mMeshes[0].meshPrimitives.size()
              << '\n';
    std::cout << "meshPrimitives.positions.size()="
              << mMeshes[0].meshPrimitives[0].positions.size() << '\n';
    std::cout << "meshPrimitives.normals.size()="
              << mMeshes[0].meshPrimitives[0].normals.size() << '\n';
    std::cout << "meshPrimitives.texcoords.size()="
              << mMeshes[0].meshPrimitives[0].texcoords.size() << '\n';
    std::cout << "meshPrimitives.indices.size()="
              << mMeshes[0].meshPrimitives[0].indices.size() << '\n';
    std::cout << *std::max_element(mMeshes[0].meshPrimitives[0].indices.begin(),
                                   mMeshes[0].meshPrimitives[0].indices.end())
              << '\n';
}
void MeshManager::uploadLastMesh() {
    auto &mesh = mMeshes.back();
    // get the mesh primitve offset
    size_t offset = 0;
    // count the number of already uploaded mesh primitives
    for (auto &imesh : mMeshes) {
        offset += imesh.meshPrimitives.size();
    }
    offset -= mesh.meshPrimitives.size();
    // for each mesh primitive, create a MeshPrimitiveGPU
    for (auto &meshPrimitive : mesh.meshPrimitives) {
        auto &meshGPU = mMeshesGPU[offset++];
        meshGPU.mIndexCount = meshPrimitive.indices.size();

        VkDeviceSize vertexSize = meshPrimitive.positions.size() * sizeof(float);
        // upload the positions
        vk::CreateAndUploadBuffer(
            mContext, meshPrimitive.positions.data(),
            vertexSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            meshGPU.mPositions);
        // upload the normals
        VkDeviceSize normalSize = meshPrimitive.normals.size() * sizeof(float);
        vk::CreateAndUploadBuffer(
            mContext, meshPrimitive.normals.data(),
            normalSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            meshGPU.mNormals);
        // upload the texcoords
        VkDeviceSize texcoordSize = meshPrimitive.texcoords.size() * sizeof(float);
        vk::CreateAndUploadBuffer(
            mContext, meshPrimitive.texcoords.data(),
            texcoordSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            meshGPU.mTexcoords);
        // upload the indices
        VkDeviceSize indexSize = meshPrimitive.indices.size() * sizeof(std::uint32_t);
        vk::CreateAndUploadBuffer(
            mContext, meshPrimitive.indices.data(),
            indexSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            meshGPU.mIndices);

        meshPrimitive.meshGPU = &meshGPU;
    }
}
}  // namespace vk