//
// Created by thomas on 16/12/24.
//

#include <limits>
#include "PerFrameResource.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/error.hpp"

namespace GraphicsThings
{


    PerFrameResource::PerFrameResource(lut::VulkanWindow *aWindow) :
            mWindow(aWindow),
            mNumFrames(aWindow->swapViews.size())
    {
        // create the command pool for the resources to use
        mCommandPool = lut::create_command_pool(*mWindow, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        // for each frame, create a command buffer, fence, and two semaphores
        for (std::size_t i = 0; i < mNumFrames; i++)
        {
            mCommandBuffers.emplace_back(lut::alloc_command_buffer(*mWindow, mCommandPool.handle));
            mFrameDoneFences.emplace_back(lut::create_fence(*mWindow, VK_FENCE_CREATE_SIGNALED_BIT));
            mImageAvailableSemaphores.emplace_back(lut::create_semaphore(*mWindow));
            mRenderFinishedSemaphores.emplace_back(lut::create_semaphore(*mWindow));
        }


    }



    bool PerFrameResource::proceed_to_next_frame()
    {
        // increment the frame index
        mFrameIndex++;
        mFrameIndex %= mNumFrames;
        assert(mFrameIndex < mNumFrames);

        // make sure the frame done fence is signalled
        if (auto const res = vkWaitForFences(mWindow->device, 1, &mFrameDoneFences[mFrameIndex].handle, VK_TRUE,
                                             std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res)
        {
            throw lut::Error("Can't wait for fence\n"
                             "vkWaitForFences() returned %s", lut::to_string(res).c_str()
            );
        }

        // acquire the next swapchain image, and check if it needs to be recreated
        assert(mFrameIndex < mImageAvailableSemaphores.size());
        mImageIndex = 0;
        auto const acquireRes = vkAcquireNextImageKHR(mWindow->device, mWindow->swapchain,
                                                      std::numeric_limits<std::uint64_t>::max(),
                                                      mImageAvailableSemaphores[mFrameIndex].handle, VK_NULL_HANDLE,
                                                      &mImageIndex);
        if (VK_ERROR_OUT_OF_DATE_KHR == acquireRes || VK_SUBOPTIMAL_KHR == acquireRes)
        {
            mFrameIndex--;
            mFrameIndex %= mCommandBuffers.size();
            return false;
        }
        if (VK_SUCCESS != acquireRes)
        {
            throw lut::Error("Can't acquire next image\n"
                             "vkAcquireNextImageKHR() returned %s", lut::to_string(acquireRes).c_str()
            );
        }
        // reset the fence
        if (auto const res = vkResetFences(mWindow->device, 1, &mFrameDoneFences[mFrameIndex].handle); VK_SUCCESS !=
                                                                                                       res)
        {
            throw lut::Error("Can't reset fence\n"
                             "vkResetFences() returned %s", lut::to_string(res).c_str()
            );
        }

        assert(std::size_t(mImageIndex) < mCommandBuffers.size());

        return true;
    }


    void PerFrameResource::begin_recording_commands()
    {
        // begin recording commands
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        if (auto const res = vkBeginCommandBuffer(mCommandBuffers[mImageIndex], &beginInfo); VK_SUCCESS != res)
        {
            throw lut::Error("Can't begin command buffer\n"
                             "vkBeginCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }
    }

    void PerFrameResource::end_recording_commands()
    {
        // end recording commands
        if (auto const res = vkEndCommandBuffer(mCommandBuffers[mImageIndex]); VK_SUCCESS != res)
        {
            throw lut::Error("Can't end command buffer\n"
                             "vkEndCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }
    }

    void PerFrameResource::submit_commands()
    {
        assert(std::size_t(mFrameIndex) < mRenderFinishedSemaphores.size());

        VkPipelineStageFlags waitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mCommandBuffers[mImageIndex];

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &mImageAvailableSemaphores[mFrameIndex].handle;
        submitInfo.pWaitDstStageMask = &waitPipelineStages;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &mRenderFinishedSemaphores[mFrameIndex].handle;

        if (auto const res = vkQueueSubmit(mWindow->graphicsQueue, 1, &submitInfo,
                                           mFrameDoneFences[mFrameIndex].handle); VK_SUCCESS != res)
        {
            throw lut::Error("Can't submit command buffer\n"
                             "vkQueueSubmit() returned %s", lut::to_string(res).c_str()
            );
        }

    }




} // GraphicsThings