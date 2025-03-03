#pragma once
#include "Volk.hpp"
#include "VMA.hpp"
#include <string>
#include <stdexcept>
#include <cstring>
#include "Context.hpp"

class Buffer {
  public:
    Buffer() noexcept;

    /* allocator  - Free buffer once done
            buffer	  - Initialized buffer
            allocation - Associated memroy allocation for buffer
    */

    explicit Buffer(const std::string &name, VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

    /* Delete copy and copy assignment operator */
    Buffer(const Buffer &) = delete;            // You cannot copy a buffer object
    Buffer &operator=(const Buffer &) = delete; // You cannot assign a buffer to another buffer

    /* Allow moving of Buffer object, transfers ownership of resources */
    Buffer(Buffer &&) noexcept;
    Buffer &operator=(Buffer &&) noexcept;

    void Destroy();

    template <typename T>
    void WriteToBuffer(const T &data, VkDeviceSize size_in_bytes) // write data to a buffer
    {
        if (allocation == VK_NULL_HANDLE) {
            throw std::runtime_error("Attempted to write to buffer with no valid memory allocation.");
        }

        void *mappedData = nullptr;
        if (vmaMapMemory(allocator, allocation, &mappedData) == VK_SUCCESS) {
            // Handle if it's a pointer already or not
            if constexpr (!std::is_pointer_v<T>) {
                std::memcpy(mappedData, &data, size_in_bytes);
            } else {
                std::memcpy(mappedData, data, size_in_bytes);
            }
            vmaUnmapMemory(allocator, allocation);
        } else {
            throw std::runtime_error("Failed to map memory for buffer update");
        }
    }

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    std::string name = "";
};

Buffer CreateBuffer(const std::string &name, Context const &context, VkDeviceSize bSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags memoryFlags, VmaMemoryUsage = VMA_MEMORY_USAGE_AUTO);
void CreateAndUploadBuffer(Context const &context, const void *data, VkDeviceSize size, VkBufferUsageFlags usage, Buffer &destinationBuffer);
