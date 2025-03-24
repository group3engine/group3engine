#include "DepthPrepass.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Scene.hpp"
#include "Image.hpp"
#include "Camera.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"

DepthPrepass::DepthPrepass(Context &context, std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera)
    : context{context},
      scene{scene},
      camera{camera},
      m_Pipeline{VK_NULL_HANDLE},
      m_PipelineLayout{VK_NULL_HANDLE},
      m_descriptorSetLayout{VK_NULL_HANDLE},
      m_descriptorSets{},
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
    vkDestroyDescriptorSetLayout(context.device, m_descriptorSetLayout, nullptr);
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

void DepthPrepass::Execute(VkCommandBuffer cmd) const {
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
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_descriptorSets[vkutil::currentFrame], 0, nullptr);

    // Draw front freshes
    //
    scene->DrawOpaque(cmd, m_PipelineLayout);

    // scene->RenderFrontMeshes(cmd, m_PipelineLayout);
    //
    //// Doing depth-prepass on alpha masking objects will mean discard will break later
    // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    // scene->RenderBackMeshes(cmd, m_PipelineLayout);
    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG
}

void DepthPrepass::CreatePipeline() {
    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(vkutil::MeshPushConstants)};

    auto pipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
                              .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "default.vert.spv", ShaderType::VERTEX)
                              .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                              .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
                              .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                              .SetPipelineLayout({{m_descriptorSetLayout, vkutil::materialDescriptorSetLayout}}, pushConstantRange)
                              .SetSampling(VK_SAMPLE_COUNT_1_BIT)
                              .AddBlendAttachmentState()
                              .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL) // Depth write and test enabled
                              .SetRenderPass(m_renderPass)
                              .Build();

    m_Pipeline = pipelineResult.first;
    m_PipelineLayout = pipelineResult.second;
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
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
    };

    m_descriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
}

void DepthPrepass::BuildDescriptors() {
    m_descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_descriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, m_descriptorSets);

    // Camera Transform UBO
    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = camera->GetBuffers()[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraTransform);
        vkutil::UpdateDescriptorSet(context, 0, bufferInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
}

void DepthPrepass::DestroyDescriptors() {
    vkFreeDescriptorSets(context.device, context.descriptorPool, m_descriptorSets.size(), m_descriptorSets.data());
}

void DepthPrepass::RebuildDescriptors() {
    DestroyDescriptors();
    BuildDescriptors();
}
