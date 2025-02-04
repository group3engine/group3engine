#include "vkimage.hpp"

// SOLUTION_TAGS: vulkan-(ex-[^123]|cw-.)

#include <bit>
#include <limits>
#include <vector>
#include <utility>
#include <algorithm>

#include <cstdio>
#include <cassert>
#include <cstring> // for std::memcpy()

#include <stb_image.h>

#include "error.hpp"
#include "vkutil.hpp"
#include "vkbuffer.hpp"
#include "to_string.hpp"


namespace labutils
{
    Image::Image()

    noexcept =
    default;

    Image::~Image()
    {
        if (VK_NULL_HANDLE != image)
        {
            assert(VK_NULL_HANDLE != mAllocator);
            assert(VK_NULL_HANDLE != allocation);
            vmaDestroyImage(mAllocator, image, allocation);
        }
    }

    Image::Image(VmaAllocator aAllocator, VkImage aImage, VmaAllocation aAllocation)

    noexcept
            :

            image(aImage), allocation(aAllocation), mAllocator(aAllocator)
    {}

    Image::Image(Image &&aOther)

    noexcept
            :
            image(std::exchange(aOther.image, VK_NULL_HANDLE)
            ),
            allocation(std::exchange(aOther.allocation, VK_NULL_HANDLE)
            ),
            mAllocator(std::exchange(aOther.mAllocator, VK_NULL_HANDLE)
            )
    {
    }

    Image &Image::operator=(Image &&aOther)

    noexcept
    {
        std::swap(image, aOther
                .image);
        std::swap(allocation, aOther
                .allocation);
        std::swap(mAllocator, aOther
                .mAllocator);
        return *this;
    }
}

namespace labutils
{
    Image load_image_texture2d(char const *aPath, VulkanContext const &aContext, VkCommandPool aCmdPool,
                               Allocator const &aAllocator, VkFormat aFormat C5_DBGNAME_DEFN())
    {
        // flip images vertically by default
        stbi_set_flip_vertically_on_load(1);

        // load base image
        int baseWidthi, baseHeighti, baseChannelsi;
        stbi_uc *data = stbi_load(aPath, &baseWidthi, &baseHeighti, &baseChannelsi, 4/*RGBA*/);
        if (!data)
        {
            throw Error("Can't load image '%s'", aPath);
        }
        auto const baseWidth = std::uint32_t(baseWidthi);
        auto const baseHeight = std::uint32_t(baseHeighti);

        // create staging buffer and copy image data to it
        auto const sizeInBytes = baseWidth * baseHeight * 4;

        auto staging = create_buffer(aAllocator, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        void *stagingPtr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, staging.allocation, &stagingPtr); VK_SUCCESS != res)
        {
            throw Error("Can't map memory for staging buffer\n"
                        "vmaMapMemory() returned %s", to_string(res).c_str()
            );
        }
        std::memcpy(stagingPtr, data, sizeInBytes);
        vmaUnmapMemory(aAllocator.allocator, staging.allocation);
        // free image data
        stbi_image_free(data);
        // create image
        Image ret = create_image_texture2d(aAllocator, baseWidth, baseHeight, aFormat,
                                           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                           VK_IMAGE_USAGE_TRANSFER_SRC_BIT, aContext.device);
        // create command buffer for data upload and begin recording
        VkCommandBuffer cmdBuff = alloc_command_buffer(aContext, aCmdPool);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(cmdBuff, &beginInfo); VK_SUCCESS != res)
        {
            throw Error("Can't begin command buffer\n"
                        "vkBeginCommandBuffer() returned %s", to_string(res).c_str()
            );
        }

        C5_DEBUG_SET_NAME(aContext.device, ret.image, VK_OBJECT_TYPE_IMAGE);

        // transition whole image layout to optimal for transfer so we can mipmap
        auto const mipLevels = compute_mip_level_count(baseWidth, baseHeight);
        image_barrier(cmdBuff, ret.image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
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

        vkCmdCopyBufferToImage(cmdBuff, staging.buffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        //transition base level to optimal
        image_barrier(cmdBuff, ret.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
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

            vkCmdBlitImage(cmdBuff, ret.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ret.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
            // transition current level to optimal
            image_barrier(cmdBuff, ret.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, level, 1, 0, 1});
        }
        // transition last level to shader read only optimal
        image_barrier(cmdBuff, ret.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1});
        // end recording commands
        if (auto const res = vkEndCommandBuffer(cmdBuff); VK_SUCCESS != res)
        {
            throw Error("Can't end command buffer\n"
                        "vkEndCommandBuffer() returned %s", to_string(res).c_str()
            );
        }
        // submit commands
        Fence uploadComplete = create_fence(aContext);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuff;

        if (auto const res = vkQueueSubmit(aContext.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS !=
                                                                                                           res)
        {
            throw Error("Can't submit command buffer\n"
                        "vkQueueSubmit() returned %s", to_string(res).c_str()
            );
        }
        // wait for the transfer to complete
        if (auto const res = vkWaitForFences(aContext.device, 1, &uploadComplete.handle, VK_TRUE,
                                             std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res)
        {
            throw Error("Can't wait for fence\n"
                        "vkWaitForFences() returned %s", to_string(res).c_str()
            );
        }

        C5_DEBUG_SET_NAME(aContext.device, ret.image, VK_OBJECT_TYPE_IMAGE);

        // free the command buffer
        vkFreeCommandBuffers(aContext.device, aCmdPool, 1, &cmdBuff);
        return ret;
    }

    Image
    create_image_texture2d(Allocator const &aAllocator, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat aFormat,
                           VkImageUsageFlags aUsage, VkDevice aDevice, std::uint32_t mipLevels,
                           VkImageLayout initialLayout C5_DBGNAME_DEFN())
    {
        if (mipLevels == 0)
            mipLevels = compute_mip_level_count(aWidth, aHeight);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = aFormat;
        imageInfo.extent.width = aWidth;
        imageInfo.extent.height = aHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = aUsage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = initialLayout;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.flags = 0;
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        if (auto const res = vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
                VK_SUCCESS != res)
        {
            throw Error("Can't create image\n"
                        "vmaCreateImage() returned %s", to_string(res).c_str()
            );
        }

        C5_DEBUG_SET_NAME(aDevice, image, VK_OBJECT_TYPE_IMAGE);

        return Image(aAllocator.allocator, image, allocation);
    }

    std::uint32_t compute_mip_level_count(std::uint32_t aWidth, std::uint32_t aHeight)
    {
        std::uint32_t const bits = aWidth | aHeight;
        std::uint32_t const leadingZeros = std::countl_zero(bits);
        return 32 - leadingZeros;
    }

    void
    loadTexturesToImage(Allocator const &aAllocator, VulkanContext const &aContext, /* vectors of input image paths */
                        std::vector<BakedTextureInfo> aBakedTextureInfo, /* vector of output images */
                        std::vector<Image> &aImages, /*vector of image views*/ std::vector<ImageView> &aImageViews)
    {
        CommandPool loadCmdPool = create_command_pool(aContext, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
        for (int i = 0; i < int(aBakedTextureInfo.size()); i++)
        {

            if (aBakedTextureInfo[i].space == ETextureSpace::srgb)
            {
                aImages.emplace_back(
                        load_image_texture2d(aBakedTextureInfo[i].path.c_str(), aContext, loadCmdPool.handle,
                                             aAllocator,
                                             VK_FORMAT_R8G8B8A8_SRGB));
                aImageViews.emplace_back(
                        create_image_view_texture2d(aContext, aImages[i].image, VK_FORMAT_R8G8B8A8_SRGB));
            } else/* we are in unorm space */
            {
                aImages.emplace_back(
                        load_image_texture2d(aBakedTextureInfo[i].path.c_str(), aContext, loadCmdPool.handle,
                                             aAllocator,
                                             VK_FORMAT_R8G8B8A8_UNORM));
                aImageViews.emplace_back(
                        create_image_view_texture2d(aContext, aImages[i].image, VK_FORMAT_R8G8B8A8_UNORM));
            }

        }
    }

    std::vector<VkDescriptorSet> create_image_descriptor_sets(VulkanWindow const &aWindow, DescriptorPool const &aPool,
                                                              std::vector<ImageView> const &aImageViews,
                                                              Sampler const &aSampler,
                                                              DescriptorSetLayout const &objectLayout)
    {

        // for each image view, create a descriptor set
        std::vector<VkDescriptorSet> sets;

        for (auto const &view: aImageViews)
        {
            VkDescriptorSet set = alloc_descriptor_set(aWindow, aPool, objectLayout.handle);
            VkWriteDescriptorSet desc[1]{};

            VkDescriptorImageInfo textureInfo{};
            textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureInfo.imageView = view.handle;
            textureInfo.sampler = aSampler.handle;

            desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc[0].dstSet = set;
            desc[0].dstBinding = 0;
            desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            desc[0].descriptorCount = 1;
            desc[0].pImageInfo = &textureInfo;

            constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
            vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);

            sets.emplace_back(set);
        }


        return sets;
    }

    DescriptorSetLayout create_object_descriptor_layout(VulkanWindow const &aWindow)
    {
        VkDescriptorSetLayoutBinding bindings[1]{};
        bindings[0].binding = 0; // this must match the binding in the shader
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                         res)
        {
            throw Error("Can't create descriptor set layout\n"
                        "vkCreateDescriptorSetLayout() returned %s", to_string(res).c_str()
            );
        }

        return DescriptorSetLayout(aWindow.device, layout);
    }


    DescriptorSetLayout create_postprocess_descriptor_layout(VulkanWindow const &aWindow)
    {
        // two bindings - the render texture and the depth texture
        VkDescriptorSetLayoutBinding bindings[2]{};
        bindings[0].binding = 0; // this must match the binding in the shader
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding = 1; // this must match the binding in the shader
        // descriptor is a depth texture
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                         res)
        {
            throw Error("Can't create descriptor set layout\n"
                        "vkCreateDescriptorSetLayout() returned %s", to_string(res).c_str()
            );
        }

        return DescriptorSetLayout(aWindow.device, layout);
    }

    DescriptorSetLayout create_deferred_shading_layout(VulkanWindow const &aWindow)
    {
        // 4 bindings for the G-buffer textures - depth, normal, albedo-metallic, emissive-roughness
        VkDescriptorSetLayoutBinding bindings[4]{};
        // depth
        bindings[0].binding = 0; // this must match the binding in the shader
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // normal
        bindings[1].binding = 1; // this must match the binding in the shader
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // albedo-metallic
        bindings[2].binding = 2; // this must match the binding in the shader
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // emissive-roughness
        bindings[3].binding = 3; // this must match the binding in the shader
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                         res)
        {
            throw Error("Can't create descriptor set layout\n"
                        "vkCreateDescriptorSetLayout() returned %s", to_string(res).c_str()
            );
        }

        return DescriptorSetLayout(aWindow.device, layout);
    }



    DescriptorSetLayout create_material_descriptor_layout(VulkanWindow const &aWindow)
    {
        // base colour, roughness, metalness, alphamask, normalmap, emissive
        VkDescriptorSetLayoutBinding bindings[6]{};
        // base colour
        bindings[0].binding = 0; // this must match the binding in the shader
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // roughness
        bindings[1].binding = 1; // this must match the binding in the shader
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // metalness
        bindings[2].binding = 2; // this must match the binding in the shader
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // alphamask
        bindings[3].binding = 3; // this must match the binding in the shader
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // normalmap
        bindings[4].binding = 4; // this must match the binding in the shader
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // emissive
        bindings[5].binding = 5; // this must match the binding in the shader
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                         res)
        {
            throw Error("Can't create descriptor set layout\n"
                        "vkCreateDescriptorSetLayout() returned %s", to_string(res).c_str()
            );
        }

        return DescriptorSetLayout(aWindow.device, layout);
    }

    VkDescriptorSet create_material_descriptor_set(VulkanWindow const &aWindow, DescriptorPool const &aPool,
                                                   BakedMaterialInfo const &aMaterialInfo,
                                                   std::vector<ImageView> const &aImageViews,
                                                   Sampler const &aSampler,
                                                   DescriptorSetLayout const &aMaterialLayout)
    {
        // allocate the descriptor set for this material
        VkDescriptorSet set = alloc_descriptor_set(aWindow, aPool, aMaterialLayout.handle);
        // write the descriptor set
        VkWriteDescriptorSet desc[6]{};
        VkDescriptorImageInfo textureInfo[6]{};
        // write the texture infos
        for (int i = 0; i < 6; i++)
        {
            textureInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            // if the material index is out of bounds, use the base colour (0 index)
            if (aMaterialInfo[i] < aImageViews.size())
            {
                textureInfo[i].imageView = aImageViews[aMaterialInfo[i]].handle;
                textureInfo[i].sampler = aSampler.handle;
            } else
            {
                textureInfo[i].imageView = aImageViews[aMaterialInfo.baseColorTextureId].handle;
                textureInfo[i].sampler = aSampler.handle;
            }
        }
        // write the descriptor sets
        for (int i = 0; i < 6; i++)
        {
            desc[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            desc[i].dstSet = set;
            desc[i].dstBinding = i;
            desc[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            desc[i].descriptorCount = 1;
            desc[i].pImageInfo = &textureInfo[i];
        }
        // update the descriptor sets
        constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
        vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);

        return set;
    }


}
