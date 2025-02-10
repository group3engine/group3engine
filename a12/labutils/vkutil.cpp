#include "vkutil.hpp"

// SOLUTION_TAGS: vulkan-(ex-[^1]|cw-.)

#include <vector>

#include <cstdio>
#include <cassert>

#include "error.hpp"
#include "to_string.hpp"

namespace labutils
{
    ShaderModule load_shader_module(VulkanContext const &aContext, char const *aSpirvPath C5_DBGNAME_DEFN())
    {
        assert(aSpirvPath);

        if (std::FILE * fin = std::fopen(aSpirvPath, "rb"))
        {
            std::fseek(fin, 0, SEEK_END);
            auto const bytes = std::size_t(std::ftell(fin));
            std::fseek(fin, 0, SEEK_SET);

            // SPIR-V consists of a number of 32-bit = 4 byte words
            assert(0 == bytes % 4);
            auto const words = bytes / 4;

            std::vector <std::uint32_t> code(words);

            std::size_t offset = 0;
            while (offset != words)
            {
                auto const read = std::fread(code.data() + offset, sizeof(std::uint32_t), words - offset, fin);
                if (read == 0)
                {
                    auto const err = std::ferror(fin), eof = std::feof(fin);
                    std::fclose(fin);
                    throw Error("Error reading '%s': ferror = %d, feof = %d", aSpirvPath, err, eof);
                }
                offset += read;
            }
            std::fclose(fin);
            VkShaderModuleCreateInfo moduleInfo{};
            moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleInfo.codeSize = bytes;
            moduleInfo.pCode = code.data();

            VkShaderModule smod = VK_NULL_HANDLE;
            if (auto const res = vkCreateShaderModule(aContext.device, &moduleInfo, nullptr, &smod); VK_SUCCESS != res)
            {
                throw Error("Can't create shader module from %s\n"
                            "vkCreateShaderModule() returned %s", aSpirvPath, to_string(res).c_str()
                );
            }
            C5_DEBUG_SET_NAME(aContext.device, smod, VK_OBJECT_TYPE_SHADER_MODULE);
            return ShaderModule(aContext.device, smod);
        }
        throw Error("Cannot open '%s' for reading", aSpirvPath);
    }


    CommandPool create_command_pool(VulkanContext const &aContext, VkCommandPoolCreateFlags aFlags C5_DBGNAME_DEFN())
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = aContext.graphicsFamilyIndex;
        poolInfo.flags = aFlags;

        VkCommandPool cpool = VK_NULL_HANDLE;
        if (auto const res = vkCreateCommandPool(aContext.device, &poolInfo, nullptr, &cpool); VK_SUCCESS != res)
        {
            throw Error("Can't create command pool\n"
                        "vkCreateCommandPool() returned %s", to_string(res).c_str()
            );
        }
        C5_DEBUG_SET_NAME(aContext.device, cpool, VK_OBJECT_TYPE_COMMAND_POOL);
        return CommandPool(aContext.device, cpool);

    }

    VkCommandBuffer alloc_command_buffer(VulkanContext const &aContext, VkCommandPool aCmdPool C5_DBGNAME_DEFN())
    {
        VkCommandBufferAllocateInfo cbufInfo{};
        cbufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbufInfo.commandPool = aCmdPool;
        cbufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbufInfo.commandBufferCount = 1;

        VkCommandBuffer cbuf = VK_NULL_HANDLE;
        if (auto const res = vkAllocateCommandBuffers(aContext.device, &cbufInfo, &cbuf); VK_SUCCESS != res)
        {
            throw Error("Can't allocate command buffer\n"
                        "vkAllocateCommandBuffers() returned %s", to_string(res).c_str()
            );
        }
        C5_DEBUG_SET_NAME(aContext.device, cbuf, VK_OBJECT_TYPE_COMMAND_BUFFER);
        return cbuf;
    }


    Fence create_fence(VulkanContext const &aContext, VkFenceCreateFlags aFlags C5_DBGNAME_DEFN())
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = aFlags;

        VkFence fence = VK_NULL_HANDLE;
        if (auto const res = vkCreateFence(aContext.device, &fenceInfo, nullptr, &fence); VK_SUCCESS != res)
        {
            throw Error("Can't create fence\n"
                        "vkCreateFence() returned %s", to_string(res).c_str()
            );
        }
        C5_DEBUG_SET_NAME(aContext.device, fence, VK_OBJECT_TYPE_FENCE);
        return Fence(aContext.device, fence);
    }

    Semaphore create_semaphore(VulkanContext const &aContext C5_DBGNAME_DEFN())
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore = VK_NULL_HANDLE;
        if (auto const res = vkCreateSemaphore(aContext.device, &semaphoreInfo, nullptr, &semaphore); VK_SUCCESS != res)
        {
            throw Error("Can't create semaphore\n"
                        "vkCreateSemaphore() returned %s", to_string(res).c_str()
            );
        }

        C5_DEBUG_SET_NAME(aContext.device, semaphore, VK_OBJECT_TYPE_SEMAPHORE);

        return Semaphore(aContext.device, semaphore);
    }

    void buffer_barrier(VkCommandBuffer aCmdBuff, VkBuffer aBuffer, VkAccessFlags aSrcAccessFlags,
                        VkAccessFlags aDstAccessFlags, VkPipelineStageFlags aSrcStageFlags,
                        VkPipelineStageFlags aDstStageFlags, VkDeviceSize aSize, VkDeviceSize aOffset,
                        uint32_t aSrcQueueFamilyIndex, uint32_t aDstQueueFamilyIndex)
    {
        VkBufferMemoryBarrier bufferBarrier{};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.srcAccessMask = aSrcAccessFlags;
        bufferBarrier.dstAccessMask = aDstAccessFlags;
        bufferBarrier.buffer = aBuffer;
        bufferBarrier.size = aSize;
        bufferBarrier.offset = aOffset;
        bufferBarrier.srcQueueFamilyIndex = aSrcQueueFamilyIndex;
        bufferBarrier.dstQueueFamilyIndex = aDstQueueFamilyIndex;

        vkCmdPipelineBarrier(
                aCmdBuff,
                aSrcStageFlags, aDstStageFlags,
                0,
                0, nullptr,
                1, &bufferBarrier,
                0, nullptr
        );

    }


    DescriptorPool create_descriptor_pool(VulkanContext const &aContext, std::uint32_t aMaxDescriptors,
                                          std::uint32_t aMaxSets C5_DBGNAME_DEFN())
    {
        VkDescriptorPoolSize const pools[] = {
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         aMaxDescriptors},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, aMaxDescriptors},
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = aMaxSets;
        poolInfo.poolSizeCount = sizeof(pools) / sizeof(pools[0]);
        poolInfo.pPoolSizes = pools;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkDescriptorPool pool = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorPool(aContext.device, &poolInfo, nullptr, &pool); VK_SUCCESS != res)
        {
            throw Error("Can't create descriptor pool\n"
                        "vkCreateDescriptorPool() returned %s", to_string(res).c_str()
            );
        }

        C5_DEBUG_SET_NAME(aContext.device, pool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);

        return DescriptorPool(aContext.device, pool);
    }

    VkDescriptorSet alloc_descriptor_set(VulkanContext const &aContext, DescriptorPool const &aPool,
                                         VkDescriptorSetLayout aSetLayout C5_DBGNAME_DEFN())
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = aPool.handle;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &aSetLayout;

        VkDescriptorSet set = VK_NULL_HANDLE;
        if (auto const res = vkAllocateDescriptorSets(aContext.device, &allocInfo, &set); VK_SUCCESS != res)
        {
            throw Error("Can't allocate descriptor set\n"
                        "vkAllocateDescriptorSets() returned %s", to_string(res).c_str()
            );
        }

        C5_DEBUG_SET_NAME(aContext.device, set, VK_OBJECT_TYPE_DESCRIPTOR_SET);

        return set;
    }

    ImageView
    create_image_view_texture2d(VulkanContext const &aContext, VkImage aImage, VkFormat aFormat C5_DBGNAME_DEFN())
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = aImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = aFormat;
        // get the flags from the image

        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView view = VK_NULL_HANDLE;
        if (auto const res = vkCreateImageView(aContext.device, &viewInfo, nullptr, &view); VK_SUCCESS != res)
        {
            throw Error("Can't create image view\n"
                        "vkCreateImageView() returned %s", to_string(res).c_str()
            );
        }

            C5_DEBUG_SET_NAME(aContext.device, view, VK_OBJECT_TYPE_IMAGE_VIEW);

        return ImageView(aContext.device, view);
    }

    void
    image_barrier(VkCommandBuffer aCmdBuff, VkImage aImage, VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask,
                  VkImageLayout aSrcLayout, VkImageLayout aDstLayout, VkPipelineStageFlags aSrcStageMask,
                  VkPipelineStageFlags aDstStageMask, VkImageSubresourceRange aRange)
    {
        VkImageMemoryBarrier ibarrier{};
        ibarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ibarrier.image = aImage;
        ibarrier.srcAccessMask = aSrcAccessMask;
        ibarrier.dstAccessMask = aDstAccessMask;
        ibarrier.oldLayout = aSrcLayout;
        ibarrier.newLayout = aDstLayout;
        ibarrier.subresourceRange = aRange;

        vkCmdPipelineBarrier(
                aCmdBuff,
                aSrcStageMask, aDstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &ibarrier
        );


    }



    Sampler create_default_sampler(VulkanContext const& aContext C5_DBGNAME_DEFN())
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.mipLodBias = 0.f;

        VkSampler sampler = VK_NULL_HANDLE;
        if (auto const res = vkCreateSampler(aContext.device, &samplerInfo, nullptr, &sampler); VK_SUCCESS != res)
        {
            throw Error("Unable to create sampler\n vkCreateSampler() returned %s", to_string(res).c_str());
        }

        return Sampler(aContext.device, sampler);
    }

    Sampler create_postprocess_sampler(const VulkanContext &aContext C5_DBGNAME_DEFN())
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // linear filtering needed
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        // don't repeat the texture
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.mipLodBias = 0.f;

        VkSampler sampler = VK_NULL_HANDLE;
        if (auto const res = vkCreateSampler(aContext.device, &samplerInfo, nullptr, &sampler); VK_SUCCESS != res)
        {
            throw Error("Unable to create sampler\n vkCreateSampler() returned %s", to_string(res).c_str());
        }

        return Sampler(aContext.device, sampler);
    }

}