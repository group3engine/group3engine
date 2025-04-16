#include "DepthPrepass.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Scene.hpp"
#include "Image.hpp"
#include "Camera.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"

#include "RenderPassCommon.hpp"

DepthPrepass::DepthPrepass(Context &context, Scene *scene)
    : context{context},
      m_Scene{scene},
      m_Pipeline{VK_NULL_HANDLE},
      m_PipelineLayout{VK_NULL_HANDLE},
      mPlayerDescriptorSetLayout{VK_NULL_HANDLE},
      mPlayerDescriptorSets{},
      m_renderPass{VK_NULL_HANDLE},
      m_framebuffer{VK_NULL_HANDLE},
      m_width{0},
      m_height{0} {

    m_DepthTarget = CreateImageTexture2D(
        "DepthPrepass_RT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1);

    CreateRenderPass();
    CreateFramebuffer();
    BuildDescriptorSetLayouts();
    BuildDescriptors();
    CreatePipeline();
}

DepthPrepass::~DepthPrepass() {
    m_DepthTarget.Destroy(context.device);

    vkDestroyPipeline(context.device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);
    vkDestroyRenderPass(context.device, m_renderPass, nullptr);
    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);
    vkDestroyDescriptorSetLayout(context.device, mPlayerDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, skinDescriptorSetLayout, nullptr);

    vkDestroyPipeline(context.device, m_skinnedPipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_skinnedPipeline.second, nullptr);
}

void DepthPrepass::Resize() {
    m_DepthTarget.Destroy(context.device);

    m_DepthTarget = CreateImageTexture2D(
        "DepthPrepass_RT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1);

    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);

    CreateFramebuffer();
}

void DepthPrepass::Execute(VkCommandBuffer cmd) {
    ZoneScopedN("DepthPrepass::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "DepthPrepass", tracy::Color::Crimson);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "DepthPrepass");
#endif // !DEBUG

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = m_framebuffer;
    beginInfo.renderArea.extent = context.extent;

    VkClearValue clearValues[1];
    clearValues[0].depthStencil.depth = {1.0f};
    beginInfo.clearValueCount = 1;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    size_t playerCount = m_Scene->GetActivePlayerCount();
    for (size_t playerId = 0; playerId < playerCount; ++playerId) {
        //m_Skybox->Execute(cmd, playerCount, playerId);

        // NOTE: Viewport and scissor needs to be set again after executing skybox pass
        // TODO: Investigate more
        VkViewport viewport = CalcViewport(context.extent, playerCount, playerId);
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)};
        scissor.extent = {static_cast<uint32_t>(viewport.width),
                          static_cast<uint32_t>(viewport.height)};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &mPlayerDescriptorSets[playerId][vkutil::currentFrame], 0, nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skinnedPipeline.first);
        m_Scene->DrawSkinned(cmd, m_skinnedPipeline.second);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        m_Scene->DrawOpaque(cmd, m_PipelineLayout);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        m_Scene->DrawAlphaMasked(cmd, m_PipelineLayout);
    }
    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG
}

void DepthPrepass::CreatePipeline() {
    std::vector<VkPushConstantRange> pushConstants = {

        {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
         .offset = 0,
         .size = sizeof(vkutil::MeshPushConstants)},
    };

    auto pipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(assetsPath / "shaders/" / "default.vert.spv", ShaderType::VERTEX)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({{mPlayerDescriptorSetLayout, vkutil::materialDescriptorSetLayout}}, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL) // Depth write and test enabled
        .SetRenderPass(m_renderPass)
        .Build();

    m_Pipeline = pipelineResult.first;
    m_PipelineLayout = pipelineResult.second;

    auto skinnedPipeline = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(SKINNED_VERTEX_SHADER, ShaderType::VERTEX)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout( {{mPlayerDescriptorSetLayout, vkutil::materialDescriptorSetLayout, skinDescriptorSetLayout}}, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_skinnedPipeline = skinnedPipeline;
}

void DepthPrepass::CreateRenderPass() {
    RenderPass builder(context.device, 1);

    m_renderPass = builder
        .AddAttachment(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        .SetDepthAttachmentRef(0, 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
        .AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
        .Build();
}

void DepthPrepass::CreateFramebuffer() {

    std::vector<VkImageView> attachments = {m_DepthTarget.imageView};

    VkFramebufferCreateInfo fbcInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = context.extent.width,
        .height = context.extent.height,
        .layers = 1};

    VK_CHECK(vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_framebuffer), "Failed to create depth pre-pass framebuffer.");
}

void DepthPrepass::BuildDescriptorSetLayouts() {
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        };

        mPlayerDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
        skinDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}});
    }
}

void DepthPrepass::BuildDescriptors() {
    for (size_t playerId = 0; playerId < GlobalConfig::maxPlayers; ++playerId) {
        auto &descriptorSets = mPlayerDescriptorSets[playerId];

        descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mPlayerDescriptorSetLayout,
                                       vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);

        // Camera Transform UBO
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_Scene->GetCameraBuffers(playerId)[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraTransform);
            vkutil::UpdateDescriptorSet(context, 0, bufferInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }
}
