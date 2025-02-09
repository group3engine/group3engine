//
// Created by thomas on 13/12/24.
//

#ifndef MYPROJECT_PIPELINEBASECLASS_HPP
#define MYPROJECT_PIPELINEBASECLASS_HPP

#include <volk.h>
#include "vulkan_window.hpp"

namespace lut = labutils;


namespace GraphicsThings
{
    class PipelineBaseClass
    {
    public:
        PipelineBaseClass()
        {
            allPipelines.push_back(this);
        }
        virtual ~PipelineBaseClass() = default;

        virtual void recreate_pipeline() = 0;
        [[nodiscard]] virtual VkPipeline const *get_pipeline() const = 0;
        [[nodiscard]] virtual VkPipelineLayout const *get_pipeline_layout() const = 0;
        static std::vector<PipelineBaseClass *> allPipelines;
        static void recreate_all_pipelines()
        {
            for (auto pipeline : allPipelines)
            {
                pipeline->recreate_pipeline();
            }
        }

    };
}


#endif //MYPROJECT_PIPELINEBASECLASS_HPP
