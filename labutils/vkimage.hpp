#ifndef VKIMAGE_HPP_A6C9F4C6_C25F_4B9D_B9E9_3D81400A2AF1
#define VKIMAGE_HPP_A6C9F4C6_C25F_4B9D_B9E9_3D81400A2AF1
// SOLUTION_TAGS: vulkan-(ex-[^123]|cw-.)

#include <volk.h>

#include <utility>

#include <cassert>
#include <vector>
#include <string>

#include "allocator.hpp"
#include "dbgname.h"
#include "vkobject.hpp"
#include "vulkan_window.hpp"


namespace labutils
{
    class Image
    {
    public:
        Image()

        noexcept;

        ~Image();

        explicit Image(VmaAllocator, VkImage = VK_NULL_HANDLE, VmaAllocation = VK_NULL_HANDLE)

        noexcept;

        Image(Image const &) = delete;

        Image &operator=(Image const &) = delete;

        Image(Image &&)

        noexcept;

        Image &operator=(Image &&)

        noexcept;

    public:
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

    private:
        VmaAllocator mAllocator = VK_NULL_HANDLE;
    };


    Image load_image_texture2d(char const *aPath, VulkanContext const &, VkCommandPool,
                               Allocator const &aAllocator, VkFormat C5_DBGNAME_DECL());

    Image create_image_texture2d(Allocator const &, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat,
                                 VkImageUsageFlags, VkDevice aDevice = nullptr, std::uint32_t mipLevels = 0, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED  C5_DBGNAME_DECL());

    std::uint32_t compute_mip_level_count(std::uint32_t aWidth, std::uint32_t aHeight);

    std::vector<VkDescriptorSet> create_image_descriptor_sets(VulkanWindow const &aWindow, DescriptorPool const &aPool,
                                                              std::vector<ImageView> const &aImageViews,
                                                              Sampler const &aSampler,
                                                              DescriptorSetLayout const &objectLayout);

    DescriptorSetLayout create_object_descriptor_layout(VulkanWindow const &);

    DescriptorSetLayout create_postprocess_descriptor_layout(VulkanWindow const &aWindow);
    DescriptorSetLayout create_deferred_shading_layout(VulkanWindow const &aWindow);

}

#endif // VKIMAGE_HPP_A6C9F4C6_C25F_4B9D_B9E9_3D81400A2AF1
