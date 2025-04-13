#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "SH2.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include <array>
#include <tracy/TracyVulkan.hpp>

SH::SH(Context &context, Scene *scene, Image &skybox)
    : context{context}, m_Scene{scene}, skybox{skybox} {

    m_SHUniform = CreateBuffer("SHStorageBuffer", context, sizeof(vkutil::SHCoefficients), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

    BuildDescriptorSetLayouts();
    BuildDescriptors();
    CreatePipeline();
}

SH::~SH() {
    m_SHUniform.Destroy();
    vkDestroyPipeline(context.device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, mDescriptorSetLayout, nullptr);
}

void SH::Execute(VkCommandBuffer cmd) {
    ZoneScopedN("ComputeSH::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "ComputeSH", tracy::Color::Gold);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "ComputeSH");
#endif // !DEBUG
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &mDescriptorSets[vkutil::currentFrame], 0, nullptr);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    vkCmdDispatch(cmd, 64, 1, 1);
}

void SH::CreatePipeline() {

    auto pipelineResult = PipelineBuilder(context.device, PipelineType::COMPUTE, VertexBinding::NONE, 0)
        .AddShader(assetsPath / "shaders" / "SH2.comp.spv", ShaderType::COMPUTE)
        .SetPipelineLayout({{mDescriptorSetLayout}})
        .Build();

    m_Pipeline = pipelineResult.first;
    m_PipelineLayout = pipelineResult.second;
}

void SH::BuildDescriptorSetLayouts() {
    // Build descriptor set layout
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT),
            vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
        };
        mDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    }
}

void SH::BuildDescriptors() {
    {
        mDescriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, mDescriptorSets);

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorImageInfo imageInfo = {
                .sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = skybox.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            vkutil::UpdateDescriptorSet(context, 0, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }

       for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo =
            {
                .buffer = m_SHUniform.buffer,
                .offset = 0,
                .range = sizeof(vkutil::SHCoefficients)
            };

            vkutil::UpdateDescriptorSet(context, 1, bufferInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        }
    }
}
