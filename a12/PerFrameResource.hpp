//
// Created by thomas on 16/12/24.
//

#ifndef MYPROJECT_PERFRAMERESOURCE_HPP
#define MYPROJECT_PERFRAMERESOURCE_HPP

#include <cstdlib>
#include <vector>
#include "vkobject.hpp"
#include "vulkan_context.hpp"

#include "dbgname.h"
#include "vulkan_window.hpp"

namespace lut = labutils;


namespace GraphicsThings
{

    class PerFrameResource
    {
        // member functions
    public:

        explicit PerFrameResource(lut::VulkanWindow *aWindow);
        ~PerFrameResource() = default;

        [[nodiscard]] VkCommandBuffer get_command_buffer() const { return mCommandBuffers[mImageIndex]; }
        [[nodiscard]] size_t get_frame_index() const { return mFrameIndex; }
        [[nodiscard]] VkSemaphore get_render_finished_semaphore() const { return mRenderFinishedSemaphores[mFrameIndex].handle; }
        [[nodiscard]] std::uint32_t get_image_index() const { return mImageIndex; }

        // method to set the current command buffer to the recording state
        void begin_recording_commands();


        // method to set the current command buffer out of the recording state
        void end_recording_commands();


        // method to submit the current command buffer
        void submit_commands();


        // method to increment the frame index
        bool proceed_to_next_frame();



    private:
        // reference to the window
        lut::VulkanWindow const *mWindow;

        lut::CommandPool mCommandPool;

        std::size_t mFrameIndex = 0;
        std::vector<VkCommandBuffer> mCommandBuffers;
        std::vector<lut::Fence> mFrameDoneFences;
        std::vector<lut::Semaphore> mImageAvailableSemaphores;
        std::vector<lut::Semaphore> mRenderFinishedSemaphores;



        size_t mNumFrames;
        std::uint32_t mImageIndex = 0;

    };

} // GraphicsThings

#endif //MYPROJECT_PERFRAMERESOURCE_HPP
