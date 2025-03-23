#include "GBuffer.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Scene.hpp"
#include "Pipeline.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"
#include "RenderPass.hpp"
#include "Camera.hpp"

GBuffer::GBuffer(Context& context, std::shared_ptr<Scene>& scene, std::shared_ptr<Camera>& camera) : context{ context }, scene{ scene }, camera{ camera }
{
    m_width = context.extent.width;
    m_height = context.extent.height;

    m_RenderTarget = CreateImageTexture2D(
        "GBuffer_MetalRoughness_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_R8G8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    m_DepthTarget = CreateImageTexture2D(
        "GBuffer_Depth_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1
    );

    BuildDescriptorSetLayouts();
    BuildDescriptors();
    CreateRenderPass();
    CreateFramebuffer();
    CreatePipeline();
}

GBuffer::~GBuffer()
{
    m_RenderTarget.Destroy(context.device);
    m_DepthTarget.Destroy(context.device);

    vkDestroyPipeline(context.device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);

    vkDestroyPipeline(context.device, m_AlphaMaskingPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_AlphaMaskingPipelineLayout, nullptr);

    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);
    vkDestroyRenderPass(context.device, m_renderPass, nullptr);
    vkDestroyDescriptorSetLayout(context.device, m_descriptorSetLayout, nullptr);
}

void GBuffer::Resize()
{
    m_width = context.extent.width;
    m_height = context.extent.height;

    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);

    m_RenderTarget.Destroy(context.device);
    m_DepthTarget.Destroy(context.device);

    m_RenderTarget = CreateImageTexture2D(
        "GBuffer_MetalRoughness_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_R8G8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    m_DepthTarget = CreateImageTexture2D(
        "GBuffer_Depth_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1
    );


    CreateFramebuffer();
}

void GBuffer::Execute(VkCommandBuffer cmd)
{
    ZoneScopedN("GBuffer::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "Thin-G-Buffer", tracy::Color::Coral);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "Thin-G-Buffer");
#endif // !DEBUG

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = m_framebuffer;
    beginInfo.renderArea.extent = context.extent;

    VkClearValue clearValues[2];
    clearValues[0].color = { {0.0f, 0.0f } };
    clearValues[1].depthStencil.depth = { 1.0f };
    beginInfo.clearValueCount = 2;
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
    scissor.offset = { 0,0 };
    scissor.extent = { context.extent.width, context.extent.height };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_descriptorSets[vkutil::currentFrame], 0, nullptr);

    // Draw front freshes
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    scene->DrawOpaque(cmd, m_PipelineLayout);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_AlphaMaskingPipeline);
    scene->DrawAlphaMasked(cmd, m_AlphaMaskingPipelineLayout);

    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif
}

void GBuffer::CreatePipeline()
{
    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(vkutil::MeshPushConstants)
    };

    // G-Buffer for non-alpha material meshes
    auto gBufferPipelineRes =
        PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "default.vert.spv", ShaderType::VERTEX)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "gbuffer.frag.spv", ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({ {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR} })
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({ {m_descriptorSetLayout, vkutil::materialDescriptorSetLayout} }, pushConstantRange)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_Pipeline		 = gBufferPipelineRes.first;
    m_PipelineLayout = gBufferPipelineRes.second;

    // G-Buffer alpha masking
    auto gBufferAlphaMaskingPipelineRes =
        PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "default.vert.spv", ShaderType::VERTEX)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "gbuffer_alpha.frag.spv", ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({ {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR} })
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({ {m_descriptorSetLayout, vkutil::materialDescriptorSetLayout} }, pushConstantRange)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_AlphaMaskingPipeline		 = gBufferAlphaMaskingPipelineRes.first;
    m_AlphaMaskingPipelineLayout = gBufferAlphaMaskingPipelineRes.second;
}

void GBuffer::CreateRenderPass()
{
    RenderPass builder(context.device, 1);

    m_renderPass = builder
        .AddAttachment(VK_FORMAT_R8G8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .AddAttachment(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)

        .AddColorAttachmentRef(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .SetDepthAttachmentRef(0, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)

        // External -> 0 : Color
        .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // 0 -> External : Color : Wait for color writing to finish on the attachment before the fragment shader tries to read from it
        .AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // External -> 0 : Depth
        .AddDependency(VK_SUBPASS_EXTERNAL, 0,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // Written to during late fragment
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)

        // 0 -> External : Depth
        .AddDependency(0, VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)

        .Build();
}

void GBuffer::CreateFramebuffer()
{
    // Framebuffer
    std::vector<VkImageView> attachments = {
        m_RenderTarget.imageView,
        m_DepthTarget.imageView
    };

    VkFramebufferCreateInfo fbcInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = m_width,
        .height = m_height,
        .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_framebuffer), "Failed to create G-buffer pass framebuffer.");
}

void GBuffer::BuildDescriptorSetLayouts() {
    // Set = 0, binding 0 = cameraUBO, binding = 1 = textures
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // SceneUBO (projection, view etc..)
    };

    m_descriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
}

void GBuffer::BuildDescriptors()
{
    m_descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_descriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, m_descriptorSets);

    // Camera Transform UBO
    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = camera->GetBuffers()[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraTransform);
        vkutil::UpdateDescriptorSet(context, 0, bufferInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
}

void GBuffer::DestroyDescriptors() {
    vkFreeDescriptorSets(context.device, context.descriptorPool, m_descriptorSets.size(), m_descriptorSets.data());
}

void GBuffer::RebuildDescriptors() {
    DestroyDescriptors();
    BuildDescriptors();
}

void GBuffer::Update()
{

}

