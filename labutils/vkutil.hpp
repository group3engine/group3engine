#ifndef VKUTIL_HPP_9DE3C6CC_921D_46FD_8452_A7F18E276E2A
#define VKUTIL_HPP_9DE3C6CC_921D_46FD_8452_A7F18E276E2A
// SOLUTION_TAGS: vulkan-(ex-[^1]|cw-.)

#include <volk.h>

#include "vkobject.hpp"
#include "vulkan_context.hpp"
#include "dbgname.h"


namespace labutils
{
	ShaderModule load_shader_module( VulkanContext const&, char const* aSpirvPath C5_DBGNAME_DECL() );

	CommandPool create_command_pool( VulkanContext const&, VkCommandPoolCreateFlags = 0 C5_DBGNAME_DECL() );
	VkCommandBuffer alloc_command_buffer( VulkanContext const&, VkCommandPool C5_DBGNAME_DECL() );

	Fence create_fence( VulkanContext const&, VkFenceCreateFlags = 0 C5_DBGNAME_DECL() );
	Semaphore create_semaphore( VulkanContext const& C5_DBGNAME_DECL() );

    void buffer_barrier(VkCommandBuffer, VkBuffer, VkAccessFlags aSrcAccessFlags, VkAccessFlags aDstAccessFlags, VkPipelineStageFlags aSrcStageFlags, VkPipelineStageFlags aDstStageFlags, VkDeviceSize aSize = VK_WHOLE_SIZE, VkDeviceSize aOffset = 0, uint32_t aSrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t aDstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

    DescriptorPool create_descriptor_pool(VulkanContext const&, std::uint32_t aMaxDescriptors = 2048, std::uint32_t aMaxSets = 1024 C5_DBGNAME_DECL());
    VkDescriptorSet alloc_descriptor_set(VulkanContext const&, DescriptorPool const&, VkDescriptorSetLayout C5_DBGNAME_DECL());
    ImageView create_image_view_texture2d( VulkanContext const&, VkImage, VkFormat aFormat C5_DBGNAME_DECL() );
    void
    image_barrier(VkCommandBuffer aCmdBuff, VkImage aImage, VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask,
                  VkImageLayout aSrcLayout, VkImageLayout aDstLayout, VkPipelineStageFlags aSrcStageMask,
                  VkPipelineStageFlags aDstStageMask, VkImageSubresourceRange aRange);
    Sampler create_default_sampler(VulkanContext const& aContext C5_DBGNAME_DECL());
    Sampler create_postprocess_sampler(VulkanContext const& aContext C5_DBGNAME_DECL());


}
#endif // VKUTIL_HPP_9DE3C6CC_921D_46FD_8452_A7F18E276E2A
