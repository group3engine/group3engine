//
// Created by thomas on 06/02/25.
//

#include "TextureManager.hpp"
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkimage.hpp"
#include <cstring>
void TextureManager::addTexture(Image aCPUImage) {
    // check if the texture already exists, if it does, yay :)
        if(mTextureMap.find(aCPUImage.name) != mTextureMap.end()) {
                return;
        }
        auto const baseWidth = std::uint32_t(aCPUImage.width);
        auto const baseHeight = std::uint32_t(aCPUImage.height);

        // create staging buffer and copy image data to it
        auto const sizeInBytes = baseWidth * baseHeight * 4;

        auto staging = create_buffer(mAllocator, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        void *stagingPtr = nullptr;
        if (auto const res = vmaMapMemory(mAllocator.allocator, staging.allocation, &stagingPtr); VK_SUCCESS != res)
        {
            throw lut::Error("Can't map memory for staging buffer\n"
                        "vmaMapMemory() returned %s", lut::to_string(res).c_str()
            );
        }
        std::memcpy(stagingPtr, aCPUImage.data, sizeInBytes);
        vmaUnmapMemory(mAllocator.allocator, staging.allocation);
        // create image
        lut::Image ret = lut::create_image_texture2d(mAllocator, baseWidth, baseHeight, VK_FORMAT_R8G8B8A8_UNORM,
                                           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mContext.device);


        // upload the image to the GPU
        // create command buffer
        VkCommandBuffer cmdBuffer = lut::alloc_command_buffer(mContext, mCommandPool.handle);
        // begin command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(cmdBuffer, &beginInfo); VK_SUCCESS != res)
        {
            throw lut::Error("Can't begin command buffer\n"
                        "vkBeginCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }

        // transition whole image layout to optimal for transfer so we can mipmap
        auto const mipLevels = lut::compute_mip_level_count(baseWidth, baseHeight);
        lut::image_barrier(cmdBuffer, ret.image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1});
        // upload image data
        VkBufferImageCopy copy;
        copy.bufferOffset = 0;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageOffset = VkOffset3D{0, 0, 0};
        copy.imageExtent = VkExtent3D{baseWidth, baseHeight, 1};

        vkCmdCopyBufferToImage(cmdBuffer, staging.buffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        //transition base level to optimal
        lut::image_barrier(cmdBuffer, ret.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
        // process all mipmap levels
        uint32_t width = baseWidth, height = baseHeight;
        for (uint32_t level = 1; level < mipLevels; level++)
        {
            // Blit previous level to current level
            VkImageBlit blit{};
            blit.srcSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1};
            blit.srcOffsets[0] = VkOffset3D{0, 0, 0};
            blit.srcOffsets[1] = VkOffset3D{int32_t(width), int32_t(height), 1};

            // next level
            width >>= 1;
            if (width == 0) width = 1;
            height >>= 1;
            if (height == 0) height = 1;
            blit.dstSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
            blit.dstOffsets[0] = VkOffset3D{0, 0, 0};
            blit.dstOffsets[1] = VkOffset3D{int32_t(width), int32_t(height), 1};

            vkCmdBlitImage(cmdBuffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ret.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
            // transition current level to optimal
            lut::image_barrier(cmdBuffer, ret.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, level, 1, 0, 1});
        }
        // transition last level to shader read only optimal
        lut::image_barrier(cmdBuffer, ret.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1});
        // end recording commands
        if (auto const res = vkEndCommandBuffer(cmdBuffer); VK_SUCCESS != res)
        {
            throw lut::Error("Can't end command buffer\n"
                        "vkEndCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }
        // submit commands
        lut::Fence uploadComplete = create_fence(mContext);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        if (auto const res = vkQueueSubmit(mContext.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS !=
                                                                                                           res)
        {
            throw lut::Error("Can't submit command buffer\n"
                        "vkQueueSubmit() returned %s", lut::to_string(res).c_str()
            );
        }
        // wait for the transfer to complete
        if (auto const res = vkWaitForFences(mContext.device, 1, &uploadComplete.handle, VK_TRUE,
                                             std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res)
        {
            throw lut::Error("Can't wait for fence\n"
                        "vkWaitForFences() returned %s", lut::to_string(res).c_str()
            );
        }


        // free the command buffer
        vkFreeCommandBuffers(mContext.device, mCommandPool.handle, 1, &cmdBuffer);

}
