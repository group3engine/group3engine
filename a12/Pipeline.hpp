//
// Created by thomas on 13/12/24.
//

#ifndef MYPROJECT_PIPELINE_HPP
#define MYPROJECT_PIPELINE_HPP

#include <tuple>
#include <functional>
#include "vkobject.hpp"
#include "vulkan_window.hpp"
#include "PipelineBaseClass.hpp"

namespace lut = labutils;


namespace GraphicsThings
{
    // Function to recreate the pipeline
    template<typename RecreatePipeline, typename... Args>
    class Pipeline final : public PipelineBaseClass
    {
    public:
        // constructor
        Pipeline(lut::VulkanWindow const & aWindow, VkPipelineLayout const *aPipelineLayout, VkRenderPass const &renderPass,
                    RecreatePipeline recreatePipeline, Args... extraArgs) : PipelineBaseClass(),
                mWindow(&aWindow),
                mPipelineLayout(aPipelineLayout),
                mPipeline(recreatePipeline(aWindow, renderPass, *mPipelineLayout, extraArgs...)),
                mRecreatePipeline(recreatePipeline),
                mExtraArgs(std::make_tuple(extraArgs...)),
                mRenderPass(&renderPass)
        {

        }

        Pipeline() = default;

        // destructor
        ~Pipeline() override
        {
            mPipeline = lut::Pipeline(nullptr);
        }


        void recreate_pipeline() override
        {
            // call the recreate pipeline function, unpacking the tuple of extra arguments
            auto args = std::tuple_cat(std::make_tuple(std::cref(*mWindow), std::cref(*mRenderPass), std::cref(*mPipelineLayout)), mExtraArgs);
            mPipeline = std::apply(mRecreatePipeline, args);

        }

        // getter for a pointer to the pipeline
        [[nodiscard]] VkPipeline const *get_pipeline() const override
        {
            return &mPipeline.handle;
        }

        // getter for a pointer to the pipeline layout
        [[nodiscard]] VkPipelineLayout const *get_pipeline_layout() const override
        {
            return mPipelineLayout;
        }


    private:
        // pointer to the window
        lut::VulkanWindow const *mWindow{};
        // pointer to the pipeline layout for this pipeline
        VkPipelineLayout const *mPipelineLayout{};

        // the lut::Pipeline object
        lut::Pipeline mPipeline;

        // a pointer to the pipeline recreation function
        RecreatePipeline mRecreatePipeline;

        // the arguments to the pipeline recreation function
        std::tuple<Args...> mExtraArgs;

        // pointer to the render pass that uses this pipeline
        VkRenderPass const *mRenderPass{};


    };

} // GraphicsThings

#endif //MYPROJECT_PIPELINE_HPP
