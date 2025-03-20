#include "Bloom.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include <array>
#include "Utils.hpp"

Bloom::Bloom(Context &context, Image &inputImage)
    : context{context},
      inputImage{inputImage},
      m_renderPass{VK_NULL_HANDLE},
      m_HorizontalBlurFramebuffer{VK_NULL_HANDLE},
      m_VerticalBlurFramebuffer{VK_NULL_HANDLE},
      m_HorizontalBlurPipeline{VK_NULL_HANDLE},
      m_HorizontalBlurPipelineLayout{VK_NULL_HANDLE},
      m_HorizontalBlurDescriptorSetLayout{VK_NULL_HANDLE},
      m_VerticalBlurPipeline{VK_NULL_HANDLE},
      m_VerticalBlurPipelineLayout{VK_NULL_HANDLE},
      m_VerticalBlurDescriptorSetLayout{VK_NULL_HANDLE},
      m_width{0},
      m_height{0} {
    m_width = context.extent.width;
    m_height = context.extent.height;

    m_BloomBlurXRT = CreateImageTexture2D(
        "Bloom_Blur_X_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    m_BloomBlurYRT = CreateImageTexture2D(
        "Bloom_Blur_Y_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    BuildHorizontalBlurDescriptors();
    BuildVerticalBlurDescriptors();

    CreateRenderPass();
    CreateFramebuffer();
    CreatePipeline();
}

Bloom::~Bloom() {
    m_BloomBlurXRT.Destroy(context.device);
    m_BloomBlurYRT.Destroy(context.device);

    vkDestroyRenderPass(context.device, m_renderPass, nullptr);
    vkDestroyFramebuffer(context.device, m_HorizontalBlurFramebuffer, nullptr);
    vkDestroyFramebuffer(context.device, m_VerticalBlurFramebuffer, nullptr);

    vkDestroyPipeline(context.device, m_HorizontalBlurPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_HorizontalBlurPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, m_HorizontalBlurDescriptorSetLayout, nullptr);

    vkDestroyPipeline(context.device, m_VerticalBlurPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_VerticalBlurPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, m_VerticalBlurDescriptorSetLayout, nullptr);
}

void Bloom::CreateFramebuffer() {
    {
        std::vector<VkImageView> attachments = {m_BloomBlurXRT.imageView};

        VkFramebufferCreateInfo fbcInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = context.extent.width,
            .height = context.extent.height,
            .layers = 1};

        VK_CHECK(vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_HorizontalBlurFramebuffer), "Failed to create Bloom: Horizontal Blur framebuffer.");
    }

    {
        std::vector<VkImageView> attachments = {m_BloomBlurYRT.imageView};

        VkFramebufferCreateInfo fbcInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = context.extent.width,
            .height = context.extent.height,
            .layers = 1};

        VK_CHECK(vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_VerticalBlurFramebuffer), "Failed to create Bloom: Horizontal Blur framebuffer.");
    }
}

void Bloom::CreateRenderPass() {
    RenderPass builder(context.device, 1);

    m_renderPass = builder
                       .AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                       .AddColorAttachmentRef(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)

                       // External -> 0 : Color
                       .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT)

                       // 0 -> External : Color : Wait for color writing to finish on the attachment before the fragment shader tries to read from it
                       .AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT)
                       .Build();

    context.SetObjectName(context.device, (uint64_t)m_renderPass, VK_OBJECT_TYPE_RENDER_PASS, "BloomRenderPass");
}

// dstStage is where other operations will begin once srcStage is finished
void Bloom::Execute(VkCommandBuffer cmd) {
    ZoneScopedN("Bloom::Execute");

    RenderHorizontalBlur(cmd);
    RenderVerticalBlur(cmd);
}

void Bloom::RenderHorizontalBlur(VkCommandBuffer cmd) {
    ZoneScopedN("Bloom::RenderHorizontalBlur");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "BloomHorizontalBlur", tracy::Color::LimeGreen);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "BloomHorizontalBlur");
#endif

    VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBegin.renderPass = m_renderPass;
    rpBegin.framebuffer = m_HorizontalBlurFramebuffer;
    rpBegin.renderArea.extent = {m_width, m_height};

    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = clearValues;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_width;
    viewport.height = (float)m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_HorizontalBlurPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_HorizontalBlurPipelineLayout, 0, 1, &m_HorizontalBlurDescriptorSets[vkutil::currentFrame], 0, nullptr);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_HorizontalBlurPipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG
}

void Bloom::RenderVerticalBlur(VkCommandBuffer cmd) {
    ZoneScopedN("Bloom::RenderVerticalBlur");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "BloomVerticalBlur", tracy::Color::Blue);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "BloomVerticalBlur");
#endif // !DEBUG

    VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBegin.renderPass = m_renderPass;
    rpBegin.framebuffer = m_VerticalBlurFramebuffer;
    rpBegin.renderArea.extent = {m_width, m_height};

    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = clearValues;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_width;
    viewport.height = (float)m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VerticalBlurPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VerticalBlurPipelineLayout, 0, 1, &m_VerticalBlurDescriptorSets[vkutil::currentFrame], 0, nullptr);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VerticalBlurPipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG
}

void Bloom::Resize() {
    m_width = context.extent.width;
    m_height = context.extent.height;

    m_BloomBlurXRT.Destroy(context.device);
    m_BloomBlurYRT.Destroy(context.device);

    vkDestroyFramebuffer(context.device, m_HorizontalBlurFramebuffer, nullptr);
    vkDestroyFramebuffer(context.device, m_VerticalBlurFramebuffer, nullptr);

    m_BloomBlurXRT = CreateImageTexture2D(
        "Bloom_Blur_X_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    m_BloomBlurYRT = CreateImageTexture2D(
        "Bloom_Blur_Y_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    CreateFramebuffer();

    for (size_t i = 0; i < vkutil::NUM_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = inputImage.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, m_HorizontalBlurDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    // Vertical
    for (size_t i = 0; i < vkutil::NUM_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = m_BloomBlurXRT.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, m_VerticalBlurDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
}

void Bloom::Update() {
}

void Bloom::CreatePipeline() {
    auto pipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::NONE, 0)
                              .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "fs_tri.vert.spv", ShaderType::VERTEX)
                              .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "bloom_blur_x.frag.spv", ShaderType::FRAGMENT)
                              .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                              .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
                              .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                              .SetPipelineLayout({{m_HorizontalBlurDescriptorSetLayout}})
                              .SetSampling(VK_SAMPLE_COUNT_1_BIT)
                              .AddBlendAttachmentState()
                              .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL) // Turn depth read and write OFF ========
                              .SetRenderPass(m_renderPass)
                              .Build();

    m_HorizontalBlurPipeline = pipelineResult.first;
    m_HorizontalBlurPipelineLayout = pipelineResult.second;

    auto verticalPipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::NONE, 0)
                                      .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "fs_tri.vert.spv", ShaderType::VERTEX)
                                      .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "bloom_blur_y.frag.spv", ShaderType::FRAGMENT)
                                      .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                      .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
                                      .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                      .SetPipelineLayout({{m_VerticalBlurDescriptorSetLayout}})
                                      .SetSampling(VK_SAMPLE_COUNT_1_BIT)
                                      .AddBlendAttachmentState()
                                      .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL) // Turn depth read and write OFF ========
                                      .SetRenderPass(m_renderPass)
                                      .Build();

    m_VerticalBlurPipeline = verticalPipelineResult.first;
    m_VerticalBlurPipelineLayout = verticalPipelineResult.second;
}

void Bloom::BuildHorizontalBlurDescriptors() {
    m_HorizontalBlurDescriptorSets.resize(vkutil::NUM_FRAMES_IN_FLIGHT);
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
            vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
        };

        m_HorizontalBlurDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    }

    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_HorizontalBlurDescriptorSetLayout, vkutil::NUM_FRAMES_IN_FLIGHT, m_HorizontalBlurDescriptorSets);

    for (size_t i = 0; i < vkutil::NUM_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = inputImage.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, m_HorizontalBlurDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
}

void Bloom::BuildVerticalBlurDescriptors() {
    m_VerticalBlurDescriptorSets.resize(vkutil::NUM_FRAMES_IN_FLIGHT);
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        };

        m_VerticalBlurDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    }

    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_VerticalBlurDescriptorSetLayout, vkutil::NUM_FRAMES_IN_FLIGHT, m_VerticalBlurDescriptorSets);

    for (size_t i = 0; i < vkutil::NUM_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = m_BloomBlurXRT.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, m_VerticalBlurDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
}
