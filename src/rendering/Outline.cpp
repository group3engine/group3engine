//
// Created by thomas on 13/04/25.
//

#include "Outline.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Scene.hpp"

Outline::Outline(Context &context, Scene *scene, Image &depthBuffer, Image &normalRoughnessImage) :
    m_context{context}, m_Scene{scene}, m_depthBuffer{depthBuffer}, m_normalRoughness{normalRoughnessImage} {

    m_width = context.extent.width;
    m_height = context.extent.height;

    m_RenderTarget = CreateImageTexture2D(
        "Outline_RenderTarget",
        context,
        m_width,
        m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    m_outlineUniform.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    for (auto &buffer : m_outlineUniform) {
        buffer = CreateBuffer("OutlineSettingsUBO", context, sizeof(vkutil::OutlineSettings), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    BuildDescriptorSetLayouts();
    BuildDescriptors();
    CreateRenderPass();
    CreateFramebuffer();
    CreatePipeline();
}
Outline::~Outline() {
    for (auto &buffer : m_outlineUniform) {
        buffer.Destroy();
    }
    m_RenderTarget.Destroy(m_context.device);
    vkDestroyPipeline(m_context.device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(m_context.device, m_PipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_context.device, m_descriptorSetLayout, nullptr);
    vkDestroyFramebuffer(m_context.device, m_framebuffer, nullptr);
    vkDestroyRenderPass(m_context.device, m_renderPass, nullptr);
}

void Outline::Update()
{
    m_outlineUniform[vkutil::currentFrame].WriteToBuffer(vkutil::outlineSettings, sizeof(vkutil::OutlineSettings));
}

void Outline::Resize()
{
    m_width = m_context.extent.width;
    m_height = m_context.extent.height;

    m_RenderTarget.Destroy(m_context.device);

    m_RenderTarget = CreateImageTexture2D(
        "Outline_RenderTarget",
        m_context,
        m_width,
        m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {.sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = m_depthBuffer.imageView,
                .imageLayout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(m_context, 0, imageInfo, m_descriptorSets[i],
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {.sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = m_normalRoughness.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(m_context, 1, imageInfo, m_descriptorSets[i],
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    vkDestroyFramebuffer(m_context.device, m_framebuffer, nullptr);
    CreateFramebuffer();
}

void Outline::Execute(VkCommandBuffer cmd)
{
    ZoneScopedN("Outline::Execute");
    TracyVkZoneC(m_context.tracyContexts[vkutil::currentFrame], cmd, "Outline", tracy::Color::Burlywood);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "Outline");
#endif // !DEBUG

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {m_width, m_height};

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = m_context.extent.width;
    viewport.height = m_context.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_context.extent.width, m_context.extent.height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
                            &m_descriptorSets[vkutil::currentFrame], 0, nullptr);

    vkCmdDraw(cmd, 3, 1, 0, 0);


    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG

}

void Outline::CreateFramebuffer()
{
    std::vector<VkImageView> attachments = { m_RenderTarget.imageView };

    VkFramebufferCreateInfo fbcInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = m_width,
        .height = m_height,
        .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(m_context.device, &fbcInfo, nullptr, &m_framebuffer), "Failed to create Outline framebuffer.");
}

void Outline::CreatePipeline()
{
    auto pipelineResult = PipelineBuilder(m_context.device, PipelineType::GRAPHICS, VertexBinding::NONE, 0)
            .AddShader(assetsPath / "shaders/" / "fs_tri.vert.spv", ShaderType::VERTEX)
            .AddShader(assetsPath / "shaders/" / "outline.frag.spv", ShaderType::FRAGMENT)
            .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .SetDynamicState({ {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR} })
            .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetPipelineLayout({ m_descriptorSetLayout })
            .SetSampling(VK_SAMPLE_COUNT_1_BIT)
            .AddBlendAttachmentState()
            .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER)
            .SetRenderPass(m_renderPass)
            .Build();
    m_Pipeline = pipelineResult.first;
    m_PipelineLayout = pipelineResult.second;
}

void Outline::CreateRenderPass()
{
    RenderPass builder(m_context.device, 1);

    m_renderPass = builder
        .AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .AddColorAttachmentRef(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)

        // External -> 0 : Color
        .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // 0 -> External : Color : Wait for color writing to finish on the attachment before the fragment shader tries to read from it
        .AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT)
        .Build();

    m_context.SetObjectName(m_context.device, (uint64_t)m_renderPass, VK_OBJECT_TYPE_RENDER_PASS, "OutlineRenderPass");

}

void Outline::BuildDescriptorSetLayouts()
{
    // Build descriptor set layout
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
            vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
            vkutil::CreateDescriptorBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        };
        m_descriptorSetLayout = vkutil::CreateDescriptorSetLayout(m_context, bindings);
    }
}

void Outline::BuildDescriptors()
{
    m_descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    vkutil::AllocateDescriptorSets(m_context, m_context.descriptorPool, m_descriptorSetLayout,
                                   vkutil::MAX_FRAMES_IN_FLIGHT, m_descriptorSets);

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {.sampler = vkutil::clampToEdgeSamplerAniso,
                                           .imageView = m_depthBuffer.imageView,
                                           .imageLayout =
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(m_context, 0, imageInfo, m_descriptorSets[i],
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {.sampler = vkutil::clampToEdgeSamplerAniso,
                                           .imageView = m_normalRoughness.imageView,
                                           .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(m_context, 1, imageInfo, m_descriptorSets[i],
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_outlineUniform[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(vkutil::OutlineSettings);

        vkutil::UpdateDescriptorSet(m_context, 2, bufferInfo, m_descriptorSets[i],
                                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
}