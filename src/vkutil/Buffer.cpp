#include "Context.hpp"
#include "Buffer.hpp"

#include <utility>
#include "Utils.hpp"

#include <utility>

Buffer::Buffer() noexcept
    : buffer{VK_NULL_HANDLE}, allocation{VK_NULL_HANDLE}, allocator{VK_NULL_HANDLE} {
}

Buffer::Buffer(std::string name, VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, VmaAllocationInfo allocInfo, VkMemoryPropertyFlags memPropFlags)
    : buffer{buffer}, allocation{allocation}, allocator{allocator}, allocInfo{allocInfo}, name{std::move(name)}, memPropFlags{memPropFlags} {
}

void Buffer::Destroy() {
    if (buffer != VK_NULL_HANDLE) {
        assert(allocator != VK_NULL_HANDLE);
        assert(allocation != VK_NULL_HANDLE);
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
}

void Buffer::Update(const Context& context, const void *data, VkDeviceSize size_in_bytes)
{
    // based on https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html

    if(memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {

        // Allocation ended up in a mappable memory and is already mapped - write to it directly.

        // [Executed in runtime]:
        memcpy(allocInfo.pMappedData, data, size_in_bytes);
        VK_CHECK(vmaFlushAllocation(allocator, allocation, 0, VK_WHOLE_SIZE), "Failed to flush buffer");
        vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmdBuf)
        {
            VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            bufMemBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            bufMemBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
            bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.buffer = buffer;
            bufMemBarrier.offset = 0;
            bufMemBarrier.size = VK_WHOLE_SIZE;

            vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                0, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);
        });
    }
    else
    {
        // Allocation ended up in a non-mappable memory - a transfer using a staging buffer is required.
        VkBufferCreateInfo stagingBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingBufCreateInfo.size = size_in_bytes;
        stagingBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocCreateInfo = {};
        stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuf;
        VmaAllocation stagingAlloc;
        VmaAllocationInfo stagingAllocInfo;
        VK_CHECK(vmaCreateBuffer(allocator, &stagingBufCreateInfo, &stagingAllocCreateInfo,
            &stagingBuf, &stagingAlloc, &stagingAllocInfo), "Failed to create staging buffer");

        // [Executed in runtime]:
        memcpy(stagingAllocInfo.pMappedData, data, size_in_bytes);
        VK_CHECK(vmaFlushAllocation(allocator, stagingAlloc, 0, VK_WHOLE_SIZE), "Failed to flush staging buffer");
        // Check result...

        vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmdBuf)
        {
            VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            bufMemBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            bufMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.buffer = stagingBuf;
            bufMemBarrier.offset = 0;
            bufMemBarrier.size = VK_WHOLE_SIZE;

            vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);

            VkBufferCopy bufCopy = {
                0, // srcOffset
                0, // dstOffset,
                size_in_bytes, // size
            };

            vkCmdCopyBuffer(cmdBuf, stagingBuf, buffer, 1, &bufCopy);

            VkBufferMemoryBarrier bufMemBarrier2 = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            bufMemBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufMemBarrier2.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT; // We created a uniform buffer
            bufMemBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier2.buffer = buffer;
            bufMemBarrier2.offset = 0;
            bufMemBarrier2.size = VK_WHOLE_SIZE;

            vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                0, 0, nullptr, 1, &bufMemBarrier2, 0, nullptr);
        });

        // Destroy staging buffer
        vmaDestroyBuffer(allocator, stagingBuf, stagingAlloc);
    }

}

void Buffer::Upload(VkCommandBuffer cmdBuff, const void *data, VkDeviceSize size_in_bytes)
{
    // based on https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html

    if(memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        // Allocation ended up in a mappable memory and is already mapped - write to it directly.

        // [Executed in runtime]:
        memcpy(allocInfo.pMappedData, data, size_in_bytes);
        VK_CHECK(vmaFlushAllocation(allocator, allocation, 0, VK_WHOLE_SIZE), "Failed to flush buffer");

        VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        bufMemBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        bufMemBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.buffer = buffer;
        bufMemBarrier.offset = 0;
        bufMemBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            0, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);
    }
    else
    {
        // Allocation ended up in a non-mappable memory - a transfer using a staging buffer is required.
        VkBufferCreateInfo stagingBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingBufCreateInfo.size = size_in_bytes;
        stagingBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocCreateInfo = {};
        stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuf;
        VmaAllocation stagingAlloc;
        VmaAllocationInfo stagingAllocInfo;
        VK_CHECK(vmaCreateBuffer(allocator, &stagingBufCreateInfo, &stagingAllocCreateInfo,
            &stagingBuf, &stagingAlloc, &stagingAllocInfo), "Failed to create staging buffer");

        // [Executed in runtime]:
        memcpy(stagingAllocInfo.pMappedData, data, size_in_bytes);
        VK_CHECK(vmaFlushAllocation(allocator, stagingAlloc, 0, VK_WHOLE_SIZE), "Failed to flush staging buffer");
        // Check result...

        VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        bufMemBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        bufMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.buffer = stagingBuf;
        bufMemBarrier.offset = 0;
        bufMemBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);

        VkBufferCopy bufCopy = {
            0, // srcOffset
            0, // dstOffset,
            size_in_bytes, // size
        };

        vkCmdCopyBuffer(cmdBuff, stagingBuf, buffer, 1, &bufCopy);

        VkBufferMemoryBarrier bufMemBarrier2 = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        bufMemBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufMemBarrier2.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT; // We created a uniform buffer
        bufMemBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier2.buffer = buffer;
        bufMemBarrier2.offset = 0;
        bufMemBarrier2.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            0, 0, nullptr, 1, &bufMemBarrier2, 0, nullptr);

        // Destroy staging buffer
        vmaDestroyBuffer(allocator, stagingBuf, stagingAlloc);
    }
}

Buffer CreateBuffer(const std::string &name, Context const &context, VkDeviceSize bSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags memoryFlags, VmaMemoryUsage memUsage) {
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bSize,
        .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

    VmaAllocationCreateInfo allocCreateInfo = {
        .flags = memoryFlags,
        .usage = memUsage};

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfo = {};

    VK_CHECK(vmaCreateBuffer(context.allocator, &bufferInfo, &allocCreateInfo, &buffer, &allocation, &allocInfo), "Failed to create & allocate buffer");

    vmaSetAllocationName(context.allocator, allocation, name.c_str());
    context.SetObjectName(context.device, reinterpret_cast<uint64_t>(buffer), VK_OBJECT_TYPE_BUFFER, name.c_str());
    VkMemoryPropertyFlags memPropFlags;
    vmaGetAllocationMemoryProperties(context.allocator, allocation, &memPropFlags);

    return Buffer(name, context.allocator, buffer, allocation, allocInfo, memPropFlags);
}

void CreateAndUploadBuffer(Context const &context, const void *data, VkDeviceSize size, VkBufferUsageFlags usage, Buffer &destinationBuffer) {
    // Create the destination buffer
    destinationBuffer = CreateBuffer("buffer", context, size,
                                     usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO);
    destinationBuffer.Update(context, data, size);
}
