#include "PresentPass.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"
#include "ImGuiRenderer.hpp"

/*
        This pass will just take the forward pass shading image and present it
*/

PresentPass::PresentPass(Context &context, Image &renderedScene)
    : context{context},
      renderedScene{renderedScene},
      m_pipeline{VK_NULL_HANDLE},
      m_pipelineLayout{VK_NULL_HANDLE},
      m_renderType{vkutil::renderType} {
    m_postProcessUbo.resize(vkutil::MAX_FRAMES_IN_FLIGHT);

    for (auto &buffer : m_postProcessUbo) {
        buffer = CreateBuffer("PostProcessUBO", context, sizeof(vkutil::PostProcessing), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    BuildDescriptors();
    CreatePipeline();
}

PresentPass::~PresentPass() {
    for (auto &buffer : m_postProcessUbo) {
        buffer.Destroy();
    }

    vkDestroyPipeline(context.device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, m_descriptorSetLayout, nullptr);
}

void PresentPass::Resize() {
    // Update the descriptor to the new updated re-sized renderedScene image
    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imgInfo = {
            .sampler = vkutil::repeatSampler,
            .imageView = renderedScene.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(context, 1, imgInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
}

void PresentPass::Update() {
    m_postProcessUbo[vkutil::currentFrame].WriteToBuffer(vkutil::postProcessSettings, sizeof(vkutil::PostProcessing));
}

void PresentPass::Execute(VkCommandBuffer cmd, uint32_t imageIndex) const {
    ZoneScopedN("PresentPass::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "PresentPass", tracy::Color::DimGray);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "PresentPass");
#endif // !DEBUG

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = context.renderPass;
    beginInfo.framebuffer = context.swapchainFramebuffers[imageIndex];
    beginInfo.renderArea.extent = context.extent;

    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    beginInfo.clearValueCount = 1;
    beginInfo.pClearValues = clearValues;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)context.extent.width;
    viewport.height = (float)context.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {context.extent.width, context.extent.height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[vkutil::currentFrame], 0, nullptr);

    // Draw large triangle here
    vkCmdDraw(cmd, 3, 1, 0, 0);

    ImGuiRenderer::Render(cmd, context, imageIndex);

    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif
}

void PresentPass::CreatePipeline() {

    // Create the pipeline
    auto pipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::NONE, 0)
                              .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "fs_tri.vert.spv", ShaderType::VERTEX)
                              .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "present_pass.frag.spv", ShaderType::FRAGMENT)
                              .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                              .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
                              .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                              .SetPipelineLayout({{m_descriptorSetLayout}})
                              .SetSampling(VK_SAMPLE_COUNT_1_BIT)
                              .AddBlendAttachmentState()
                              .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL) // Turn depth read and write OFF ========
                              .SetRenderPass(context.renderPass)
                              .Build();

    m_pipeline = pipelineResult.first;
    m_pipelineLayout = pipelineResult.second;
}

void PresentPass::BuildDescriptors() {
    m_descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);

    // Set = 0, binding 0 = rendered scene image
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
        vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};

    m_descriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);

    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_descriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, m_descriptorSets);

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_postProcessUbo[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(vkutil::PostProcessing);
        vkutil::UpdateDescriptorSet(context, 0, bufferInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imgInfo = {
            .sampler = vkutil::repeatSampler,
            .imageView = renderedScene.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        vkutil::UpdateDescriptorSet(context, 1, imgInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
}
