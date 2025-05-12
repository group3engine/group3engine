#include "Fog.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include <array>

Fog::Fog(Context &context, Scene *scene, Image &depthBuffer,
         Image &renderedScene, Image &skybox, const ShadowMap* shadowMapRenderPass)
    :
      context{context}, m_Scene{scene}, depthBuffer{depthBuffer},
      renderedScene{renderedScene}, normalRoughnessImage{normalRoughnessImage},
      skybox{skybox}, shadowMapRenderPass{shadowMapRenderPass} {

    m_width = context.extent.width;
    m_height = context.extent.height;

    m_RenderTarget = CreateImageTexture2D(
        "Fog_RenderTarget", context, m_width, m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, 1
    );

    m_FogUniform.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    for (auto &buffer : m_FogUniform) {
        buffer = CreateBuffer(
            "FogSettingsUBO", context, sizeof(vkutil::FogSettings),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    BuildDescriptorSetLayouts();
    BuildDescriptors();
    CreateRenderPass();
    CreateFramebuffer();
    CreatePipeline();
}

Fog::~Fog() {
    for (auto &buffer : m_FogUniform) {
        buffer.Destroy();
    }
    m_RenderTarget.Destroy(context.device);
    vkDestroyPipeline(context.device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, mPlayerDescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorSetLayout(context.device, mDescriptorSetLayout, nullptr);
    vkDestroyFramebuffer(context.device, m_Framebuffer, nullptr);
    vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
}

void Fog::Update() {

    m_FogUniform[vkutil::currentFrame].WriteToBuffer(&vkutil::fogSettings, sizeof(vkutil::FogSettings));
}

void Fog::Resize() {

    m_width = context.extent.width;
    m_height = context.extent.height;

    m_RenderTarget.Destroy(context.device);

    m_RenderTarget = CreateImageTexture2D(
        "Fog_RenderTarget", context, m_width, m_height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, 1
    );

    for (size_t playerId = 0; playerId < GlobalConfig::maxPlayers; ++playerId) {
        auto &descriptorSets = mPlayerDescriptorSets[playerId];

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_Scene->GetCameraBuffers(playerId)[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraTransform);
            vkutil::UpdateDescriptorSet(context, 0, bufferInfo,
                                        descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = depthBuffer.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_FogUniform[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(vkutil::FogSettings);

        vkutil::UpdateDescriptorSet(context, 1, bufferInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = renderedScene.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        vkutil::UpdateDescriptorSet(context, 2, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }


    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = shadowMapRenderPass->GetCascadeUniformBuffer()[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(vkutil::CascadeMatrices);

        vkutil::UpdateDescriptorSet(context, 4, bufferInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = shadowMapRenderPass->GetRenderTarget().imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
        };

        vkutil::UpdateDescriptorSet(context, 5, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    vkDestroyFramebuffer(context.device, m_Framebuffer, nullptr);
    CreateFramebuffer();
}

void Fog::Execute(VkCommandBuffer cmd) {
    ZoneScopedN("Fog::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "Fog",
                 tracy::Color::Gold);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "Fog");
#endif // !DEBUG

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_Framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {m_width, m_height};

    std::array<VkClearValue, 1> clearValues{};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = context.extent.width;
    viewport.height = context.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {context.extent.width, context.extent.height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // TODO: Can move more stuff out of these loops
    size_t playerCount = m_Scene->GetActivePlayerCount();
    for (size_t playerId = 0; playerId < playerCount; ++playerId) {


    }

//    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
//
//    vkCmdBindDescriptorSets(
//        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
//        &mPlayerDescriptorSets[0][vkutil::currentFrame], 0, nullptr);
//
//    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                            m_PipelineLayout, 1, 1,
//                            &mDescriptorSets[vkutil::currentFrame], 0, nullptr);
//
//    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG
}

void Fog::CreateFramebuffer() {
    std::vector<VkImageView> attachments = {m_RenderTarget.imageView};

    VkFramebufferCreateInfo fbcInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_RenderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = m_width,
        .height = m_height,
        .layers = 1};

    VK_CHECK(
        vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_Framebuffer),
        "Failed to create Fog framebuffer.");
}

void Fog::CreatePipeline() {

    auto pipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::NONE, 0)
            .AddShader(assetsPath / "shaders" / "fs_tri.vert.spv", ShaderType::VERTEX)
            .AddShader(assetsPath / "shaders" / "Fog.frag.spv", ShaderType::FRAGMENT)
            .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
            .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetPipelineLayout({{mPlayerDescriptorSetLayout, mDescriptorSetLayout}})
            .SetSampling(VK_SAMPLE_COUNT_1_BIT)
            .AddBlendAttachmentState()
            .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetRenderPass(m_RenderPass)
            .Build();

    m_Pipeline = pipelineResult.first;
    m_PipelineLayout = pipelineResult.second;
}

void Fog::CreateRenderPass() {
    RenderPass builder(context.device, 1);

    m_RenderPass =
        builder
            .AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT,
                           VK_ATTACHMENT_LOAD_OP_CLEAR,
                           VK_ATTACHMENT_STORE_OP_STORE,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .AddColorAttachmentRef(0, 0,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)

            // External -> 0 : Color
            .AddDependency(VK_SUBPASS_EXTERNAL, 0,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_ACCESS_SHADER_READ_BIT,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT)

            // 0 -> External : Color : Wait for color writing to finish on the
            // attachment before the fragment shader tries to read from it
            .AddDependency(0, VK_SUBPASS_EXTERNAL,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_ACCESS_SHADER_READ_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT)
            .Build();

    context.SetObjectName(context.device, (uint64_t)m_RenderPass,
                          VK_OBJECT_TYPE_RENDER_PASS, "FogRenderPass");
}

void Fog::BuildDescriptorSetLayouts() {
    // Build player descriptor set layout
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)};

        mPlayerDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    }

    // Build descriptor set layout
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
            vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
            vkutil::CreateDescriptorBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
            vkutil::CreateDescriptorBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
            vkutil::CreateDescriptorBinding(4, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT), //CSM Uniforms
            vkutil::CreateDescriptorBinding(5, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // Shadow map

        };

        mDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    }
}

void Fog::BuildDescriptors() {
    // Build player descriptors

    for (size_t playerId = 0; playerId < GlobalConfig::maxPlayers; ++playerId) {
        auto &descriptorSets = mPlayerDescriptorSets[playerId];

        descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mPlayerDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_Scene->GetCameraBuffers(playerId)[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraTransform);
            vkutil::UpdateDescriptorSet(context, 0, bufferInfo, descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }

    // Build descriptors
    {
        mDescriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, mDescriptorSets);

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorImageInfo imageInfo = {
                .sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = depthBuffer.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            };

            vkutil::UpdateDescriptorSet(context, 0, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_FogUniform[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(vkutil::FogSettings);

            vkutil::UpdateDescriptorSet(context, 1, bufferInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorImageInfo imageInfo = {
                .sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = renderedScene.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            vkutil::UpdateDescriptorSet(context, 2, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorImageInfo imageInfo = {
                .sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = skybox.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            vkutil::UpdateDescriptorSet(context, 3, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }


        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = shadowMapRenderPass->GetCascadeUniformBuffer()[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(vkutil::CascadeMatrices);

            vkutil::UpdateDescriptorSet(context, 4, bufferInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorImageInfo imageInfo = {
                .sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = shadowMapRenderPass->GetRenderTarget().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
            };

            vkutil::UpdateDescriptorSet(context, 5, imageInfo, mDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
    }
}
