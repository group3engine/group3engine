//
// Created by thomas on 05/02/25.
//

#include "MeshManager.hpp"
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vkutil.hpp"
#include "glm/glm.hpp"
#include "../labutils/dbgname.h"

#include <cstring>
#include <iostream>
#include <limits>

void MeshManager::debugOuptutMeshes() {
    std::cout << "meshes.size()=" << mMeshes.size() << '\n';
    std::cout << "meshPrimitives.size()=" << mMeshes[0].meshPrimitives.size() << '\n';
    std::cout << "meshPrimitives.positions.size()=" << mMeshes[0].meshPrimitives[0].positions.size()
              << '\n';
    std::cout << "meshPrimitives.normals.size()=" << mMeshes[0].meshPrimitives[0].normals.size()
              << '\n';
    std::cout << "meshPrimitives.texcoords.size()=" << mMeshes[0].meshPrimitives[0].texcoords.size()
              << '\n';
    std::cout << "meshPrimitives.indices.size()=" << mMeshes[0].meshPrimitives[0].indices.size()
              << '\n';
    std::cout << *std::max_element(mMeshes[0].meshPrimitives[0].indices.begin(),
                                   mMeshes[0].meshPrimitives[0].indices.end())
              << '\n';
}
void MeshManager::uploadLastMesh() {
    auto &mesh = mMeshes.back();
    // get the mesh primitve offset
    size_t offset = 0;
    // count the number of already uploaded mesh primitives
    for(auto &imesh : mMeshes) {
        offset += imesh.meshPrimitives.size();
    }
    offset -= mesh.meshPrimitives.size();
    // for each mesh primitive, create a MeshPrimitiveGPU
    for(auto &meshPrimitive : mesh.meshPrimitives) {
        auto &meshGPU = mMeshesGPU[offset++];
        meshGPU.mIndexCount = meshPrimitive.indices.size();
        // create final position, texcoord, normal and index buffers
        meshGPU.mPositions =
            lut::create_buffer(mAllocator, meshPrimitive.positions.size() * sizeof(std::uint32_t),
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        meshGPU.mNormals =
            lut::create_buffer(mAllocator, meshPrimitive.normals.size() * sizeof(std::uint32_t),
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

        if(meshPrimitive.texcoords.empty()) {
            meshPrimitive.texcoords.resize(meshPrimitive.positions.size() / 3.f * 2.f);
        }
        meshGPU.mTexcoords =
            lut::create_buffer(mAllocator, meshPrimitive.texcoords.size() * sizeof(std::uint32_t),
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        meshGPU.mIndices =
            lut::create_buffer(mAllocator, meshPrimitive.indices.size() * sizeof(std::uint32_t),
                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        char const * aDebugName = mesh.name.c_str();
        std::source_location aDbgSrcLoc = std::source_location::current();
        C5_DEBUG_SET_NAME(mContext.device, meshGPU.mPositions.buffer, VK_OBJECT_TYPE_BUFFER);
        C5_DEBUG_SET_NAME(mContext.device, meshGPU.mTexcoords.buffer, VK_OBJECT_TYPE_BUFFER);
        C5_DEBUG_SET_NAME(mContext.device, meshGPU.mNormals.buffer, VK_OBJECT_TYPE_BUFFER);
        C5_DEBUG_SET_NAME(mContext.device, meshGPU.mIndices.buffer, VK_OBJECT_TYPE_BUFFER);
        // next, create the staging buffers
        lut::Buffer posStaging = lut::create_buffer(
            mAllocator, meshPrimitive.positions.size() * sizeof(std::uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        lut::Buffer texcoordStaging = lut::create_buffer(
            mAllocator,  meshPrimitive.texcoords.size() * sizeof(std::uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        lut::Buffer normalStaging =
            lut::create_buffer(mAllocator,meshPrimitive.normals.size() * sizeof(std::uint32_t),
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        lut::Buffer indexStaging =
            lut::create_buffer(mAllocator, meshPrimitive.indices.size() * sizeof(std::uint32_t),
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        // map to retrieve a pointer to each section of the staging memory
        void *posPtr = nullptr;
        if (auto const res = vmaMapMemory(mAllocator.allocator, posStaging.allocation, &posPtr);
            VK_SUCCESS != res) {
            throw lut::Error("Can't map memory for vertex positions\n"
                             "vmaMapMemory() returned %s",
                             lut::to_string(res).c_str());
        }
        // copy the data
        std::memcpy(posPtr, meshPrimitive.positions.data(),
                    meshPrimitive.positions.size() * sizeof(std::uint32_t));
        // unmap the memory
        vmaUnmapMemory(mAllocator.allocator, posStaging.allocation);
        void *texcoordPtr = nullptr;
        if (auto const res =
                vmaMapMemory(mAllocator.allocator, texcoordStaging.allocation, &texcoordPtr);
            VK_SUCCESS != res) {
            throw lut::Error("Can't map memory for vertex texcoords\n"
                             "vmaMapMemory() returned %s",
                             lut::to_string(res).c_str());
        }
        // copy the data
        std::memcpy(texcoordPtr, meshPrimitive.texcoords.data(),
                    meshPrimitive.texcoords.size() * sizeof(std::uint32_t));
        // unmap the memory
        vmaUnmapMemory(mAllocator.allocator, texcoordStaging.allocation);
        void *normPtr = nullptr;
        if (auto const res = vmaMapMemory(mAllocator.allocator, normalStaging.allocation, &normPtr);
            VK_SUCCESS != res) {
            throw lut::Error("Can't map memory for vertex tangents\n"
                             "vmaMapMemory() returned %s",
                             lut::to_string(res).c_str());
        }
        // copy the data
        std::memcpy(normPtr, meshPrimitive.normals.data(),
                    meshPrimitive.normals.size() * sizeof(std::uint32_t));
        // unmap the memory
        vmaUnmapMemory(mAllocator.allocator, normalStaging.allocation);
        void *indexPtr = nullptr;
        if (auto const res = vmaMapMemory(mAllocator.allocator, indexStaging.allocation, &indexPtr);
            VK_SUCCESS != res) {
            throw lut::Error("Can't map memory for indices\n"
                             "vmaMapMemory() returned %s",
                             lut::to_string(res).c_str());
        }
        // copy the data
        std::memcpy(indexPtr, meshPrimitive.indices.data(),
                    meshPrimitive.indices.size() * sizeof(std::uint32_t));
        // unmap the memory
        vmaUnmapMemory(mAllocator.allocator, indexStaging.allocation);
        // prepare for the transfer
        lut::Fence uploadComplete = lut::create_fence(mContext);
        lut::CommandPool uploadPool = lut::create_command_pool(mContext);
        VkCommandBuffer uploadCmd = lut::alloc_command_buffer(mContext, uploadPool.handle);

        // record the transfer commands
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(uploadCmd, &beginInfo); VK_SUCCESS != res) {
            throw lut::Error("Can't begin command buffer\n"
                             "vkBeginCommandBuffer() returned %s",
                             lut::to_string(res).c_str());
        }

        // copy over positions
        VkBufferCopy pcopy{};
        pcopy.size = meshPrimitive.positions.size() * sizeof(std::uint32_t);

        vkCmdCopyBuffer(uploadCmd, posStaging.buffer, meshGPU.mPositions.buffer, 1, &pcopy);

        lut::buffer_barrier(uploadCmd, meshGPU.mPositions.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        // copy over texcoords
        VkBufferCopy tccopy{};
        tccopy.size = meshPrimitive.texcoords.size() * sizeof(std::uint32_t);

        vkCmdCopyBuffer(uploadCmd, texcoordStaging.buffer, meshGPU.mTexcoords.buffer, 1, &tccopy);

        lut::buffer_barrier(uploadCmd, meshGPU.mTexcoords.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        // copy over normals
        VkBufferCopy ncopy{};
        ncopy.size = meshPrimitive.normals.size() * sizeof(std::uint32_t);

        vkCmdCopyBuffer(uploadCmd, normalStaging.buffer, meshGPU.mNormals.buffer, 1, &ncopy);

        lut::buffer_barrier(uploadCmd, meshGPU.mNormals.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        // copy over indices
        VkBufferCopy icopy{};
        icopy.size = meshPrimitive.indices.size() * sizeof(std::uint32_t);

        vkCmdCopyBuffer(uploadCmd, indexStaging.buffer, meshGPU.mIndices.buffer, 1, &icopy);

        lut::buffer_barrier(uploadCmd, meshGPU.mIndices.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        if (auto const res = vkEndCommandBuffer(uploadCmd); VK_SUCCESS != res) {
            throw lut::Error("Can't end command buffer\n"
                             "vkEndCommandBuffer() returned %s",
                             lut::to_string(res).c_str());
        }

        // submit the transfer commands
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &uploadCmd;

        if (auto const res =
                vkQueueSubmit(mContext.graphicsQueue, 1, &submitInfo, uploadComplete.handle);
            VK_SUCCESS != res) {
            throw lut::Error("Can't submit command buffer\n"
                             "vkQueueSubmit() returned %s",
                             lut::to_string(res).c_str());
        }

        // wait for the transfer to complete
        if (auto const res = vkWaitForFences(mContext.device, 1, &uploadComplete.handle, VK_TRUE,
                                             std::numeric_limits<std::uint64_t>::max());
            VK_SUCCESS != res) {
            throw lut::Error("Can't wait for fence\n"
                             "vkWaitForFences() returned %s",
                             lut::to_string(res).c_str());
        }

        meshPrimitive.meshGPU = &meshGPU;
    }

}