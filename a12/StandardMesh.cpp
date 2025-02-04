//
// Created by thomas on 12/12/24.
//

#include <cstring>
#include "StandardMesh.hpp"
#include "../labutils/vkbuffer.hpp"
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vkutil.hpp"

namespace lut = labutils;


namespace GraphicsThings
{
    StandardMesh::StandardMesh(const BakedMeshData &mesh, labutils::VulkanContext const &aContext,
                               labutils::Allocator const &aAllocator, VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSet> const &aMaterialDescriptors) :
            mIndexCount(mesh.indices.size()),
            mPipelineLayout(pipelineLayout),
            mMaterialIndex(mesh.materialId),
            mMaterialDescriptors(&aMaterialDescriptors),
            mModelMatrix(mesh.modelMatrix)

    {
        // create final position, texcoord, normal and index buffers
        mPositions = lut::create_buffer(aAllocator, mesh.positions.size() * sizeof(glm::vec3),
                                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                    0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mTexcoords = lut::create_buffer(aAllocator, mesh.texcoords.size() * sizeof(glm::vec2),
                                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                    0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

        mTBNFrames = lut::create_buffer(aAllocator, mesh.compressedTBN.size() * sizeof(uint32_t),
                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                   0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        mIndices = lut::create_buffer(aAllocator, mesh.indices.size() * sizeof(std::uint32_t),
                                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                  0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        // next, create the staging buffers
        lut::Buffer posStaging = lut::create_buffer(aAllocator, mesh.positions.size() * sizeof(glm::vec3),
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        lut::Buffer texcoordStaging = lut::create_buffer(aAllocator, mesh.texcoords.size() * sizeof(glm::vec2),
                                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        lut::Buffer tbnStaging = lut::create_buffer(aAllocator, mesh.compressedTBN.size() * sizeof(uint32_t),
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        lut::Buffer indexStaging = lut::create_buffer(aAllocator, mesh.indices.size() * sizeof(std::uint32_t),
                                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        // map to retrieve a pointer to each section of the staging memory
        void *posPtr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, posStaging.allocation, &posPtr); VK_SUCCESS != res)
        {
            throw lut::Error("Can't map memory for vertex positions\n"
                             "vmaMapMemory() returned %s", lut::to_string(res).c_str()
            );
        }
        // copy the data
        std::memcpy(posPtr, mesh.positions.data(), mesh.positions.size() * sizeof(glm::vec3));
        // unmap the memory
        vmaUnmapMemory(aAllocator.allocator, posStaging.allocation);
        void *texcoordPtr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, texcoordStaging.allocation, &texcoordPtr); VK_SUCCESS !=
                                                                                                           res)
        {
            throw lut::Error("Can't map memory for vertex texcoords\n"
                             "vmaMapMemory() returned %s", lut::to_string(res).c_str()
            );
        }
        // copy the data
        std::memcpy(texcoordPtr, mesh.texcoords.data(), mesh.texcoords.size() * sizeof(glm::vec2));
        // unmap the memory
        vmaUnmapMemory(aAllocator.allocator, texcoordStaging.allocation);
        void *tbnPtr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, tbnStaging.allocation, &tbnPtr); VK_SUCCESS !=
                                                                                                 res)
        {
            throw lut::Error("Can't map memory for vertex tangents\n"
                             "vmaMapMemory() returned %s", lut::to_string(res).c_str()
            );
        }
        // copy the data
        std::memcpy(tbnPtr, mesh.compressedTBN.data(), mesh.compressedTBN.size() * sizeof(uint32_t));
        // unmap the memory
        vmaUnmapMemory(aAllocator.allocator, tbnStaging.allocation);
        void *indexPtr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, indexStaging.allocation, &indexPtr); VK_SUCCESS !=
                                                                                                     res)
        {
            throw lut::Error("Can't map memory for indices\n"
                             "vmaMapMemory() returned %s", lut::to_string(res).c_str()
            );
        }
        // copy the data
        std::memcpy(indexPtr, mesh.indices.data(), mesh.indices.size() * sizeof(std::uint32_t));
        // unmap the memory
        vmaUnmapMemory(aAllocator.allocator, indexStaging.allocation);
        // prepare for the transfer
        lut::Fence uploadComplete = lut::create_fence(aContext);
        lut::CommandPool uploadPool = lut::create_command_pool(aContext);
        VkCommandBuffer uploadCmd = lut::alloc_command_buffer(aContext, uploadPool.handle);

        // record the transfer commands
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(uploadCmd, &beginInfo); VK_SUCCESS != res)
        {
            throw lut::Error("Can't begin command buffer\n"
                             "vkBeginCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }

        // copy over positions
        VkBufferCopy pcopy{};
        pcopy.size = mesh.positions.size() * sizeof(glm::vec3);

        vkCmdCopyBuffer(uploadCmd, posStaging.buffer, mPositions.buffer, 1, &pcopy);

        lut::buffer_barrier(uploadCmd, mPositions.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        // copy over texcoords
        VkBufferCopy tccopy{};
        tccopy.size = mesh.texcoords.size() * sizeof(glm::vec2);

        vkCmdCopyBuffer(uploadCmd, texcoordStaging.buffer, mTexcoords.buffer, 1, &tccopy);

        lut::buffer_barrier(uploadCmd, mTexcoords.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);


        // copy over tbn frames
        VkBufferCopy tbncopy{};
        tbncopy.size = mesh.compressedTBN.size() * sizeof(uint32_t);

        vkCmdCopyBuffer(uploadCmd, tbnStaging.buffer, mTBNFrames.buffer, 1, &tbncopy);

        lut::buffer_barrier(uploadCmd, mTBNFrames.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        // copy over indices
        VkBufferCopy icopy{};
        icopy.size = mesh.indices.size() * sizeof(std::uint32_t);

        vkCmdCopyBuffer(uploadCmd, indexStaging.buffer, mIndices.buffer, 1, &icopy);

        lut::buffer_barrier(uploadCmd, mIndices.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        if (auto const res = vkEndCommandBuffer(uploadCmd); VK_SUCCESS != res)
        {
            throw lut::Error("Can't end command buffer\n"
                             "vkEndCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }

        // submit the transfer commands
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &uploadCmd;

        if (auto const res = vkQueueSubmit(aContext.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS !=
                                                                                                           res)
        {
            throw lut::Error("Can't submit command buffer\n"
                             "vkQueueSubmit() returned %s", lut::to_string(res).c_str()
            );
        }

        // wait for the transfer to complete
        if (auto const res = vkWaitForFences(aContext.device, 1, &uploadComplete.handle, VK_TRUE,
                                             std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res)
        {
            throw lut::Error("Can't wait for fence\n"
                             "vkWaitForFences() returned %s", lut::to_string(res).c_str()
            );
        }
    }

    void StandardMesh::record_draw(VkCommandBuffer aCmdBuff) const
    {
        // bind the descriptor set for the material
        vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1, 1,
                                &(*mMaterialDescriptors)[mMaterialIndex], 0, nullptr);
        // push the model matrix
        vkCmdPushConstants(aCmdBuff, mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &mModelMatrix);


        // bind the vertex buffers - positions, texcoords, normals
        VkBuffer buffers[] = {mPositions.buffer, mTexcoords.buffer, mTBNFrames.buffer};

        VkDeviceSize offsets[] = {0, 0, 0};

        vkCmdBindVertexBuffers(aCmdBuff, 0, sizeof(buffers) / sizeof(buffers[0]), buffers, offsets);

        // bind the index buffer
        vkCmdBindIndexBuffer(aCmdBuff, mIndices.buffer, 0, VK_INDEX_TYPE_UINT32);

        // draw the mesh
        vkCmdDrawIndexed(aCmdBuff, mIndexCount, 1, 0, 0, 0);

    }

    void StandardMesh::record_draw_shadow(VkCommandBuffer aCmdBuff) const
    {
        // push the model matrix
        vkCmdPushConstants(aCmdBuff, mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &mModelMatrix);


        VkBuffer buffers[] = {mPositions.buffer};

        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(aCmdBuff, 0, sizeof(buffers) / sizeof(buffers[0]), buffers, offsets);

        // bind the index buffer
        vkCmdBindIndexBuffer(aCmdBuff, mIndices.buffer, 0, VK_INDEX_TYPE_UINT32);

        // draw the mesh
        vkCmdDrawIndexed(aCmdBuff, mIndexCount, 1, 0, 0, 0);
    }
} // GraphicsThings