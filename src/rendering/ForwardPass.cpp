#include "ForwardPass.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Camera.hpp"
#include "Context.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"
#include "ParticleSystem.hpp"

#include "RenderPassCommon.hpp"

ForwardPass::ForwardPass(Context &context, const Image &shadowMap, Image &depthPrepass, Scene *scene, const ShadowMap* shadowMapRenderPass)
    :
      context{context},
      shadowMap{shadowMap},
      depthPrepass{depthPrepass},
      scene{scene},
      shadowMapRenderPass{shadowMapRenderPass}
{

    m_RenderTarget = CreateImageTexture2D(
        "ForwardPassRT",
        context,
        context.extent.width,
        context.extent.height,
        context.swapchainFormat,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    //m_DepthTarget = CreateImageTexture2D(
    //    "ForwardPassDepth",
    //    context,
    //    context.extent.width,
    //    context.extent.height,
    //    VK_FORMAT_D32_SFLOAT,
    //    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //    VK_IMAGE_ASPECT_DEPTH_BIT,
    //    1);

    m_NormalRoughness = CreateImageTexture2D(
        "NormalRoughnessRT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_B8G8R8A8_UNORM, // VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    m_BrightnessTexture = CreateImageTexture2D(
        "BrightnessRT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);



    CreateRenderPass();
    m_Skybox = std::make_unique<Skybox>(context, scene, m_renderPass);
    m_SHPass = std::make_unique<SH>(context, scene, m_Skybox->GetSkyBoxImage());

    vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmd) {
        m_SHPass->Execute(cmd);
    });

    BuildDescriptorSetLayouts();
    BuildDescriptors();

    CreateFramebuffer();
    CreatePipeline();



}

ForwardPass::~ForwardPass() {

    m_Skybox.reset();
    m_SHPass.reset();
    m_RenderTarget.Destroy(context.device);
    //m_DepthTarget.Destroy(context.device);
    m_NormalRoughness.Destroy(context.device);
    m_BrightnessTexture.Destroy(context.device);
    vkDestroyPipeline(context.device, m_opaquePipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_opaquePipeline.second, nullptr);
    vkDestroyPipeline(context.device, m_alphaMaskPipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_alphaMaskPipeline.second, nullptr);
    vkDestroyPipeline(context.device, m_skinnedPipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_skinnedPipeline.second, nullptr);
    vkDestroyPipeline(context.device, m_particlePipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_particlePipeline.second, nullptr);


    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);
    vkDestroyRenderPass(context.device, m_renderPass, nullptr);
    if (meshDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context.device, meshDescriptorSetLayout, nullptr);
    }

    if(skinDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context.device, skinDescriptorSetLayout, nullptr);
    }

    if(particleDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(context.device, particleDescriptorSetLayout, nullptr);
    }
}

void ForwardPass::Resize() {

    uint32_t width = context.extent.width;
    uint32_t height = context.extent.height;

    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);

    m_RenderTarget.Destroy(context.device);
    //m_DepthTarget.Destroy(context.device);
    m_NormalRoughness.Destroy(context.device);
    m_BrightnessTexture.Destroy(context.device);

    m_RenderTarget = CreateImageTexture2D(
        "ForwardPassRT",
        context,
        width,
        height,
        context.swapchainFormat,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    //m_DepthTarget = CreateImageTexture2D(
    //    "ForwardPassDepth",
    //    context,
    //    width,
    //    height,
    //    VK_FORMAT_D32_SFLOAT,
    //    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //    VK_IMAGE_ASPECT_DEPTH_BIT,
    //    1);

    m_NormalRoughness = CreateImageTexture2D(
        "NormalRoughnessRT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_B8G8R8A8_UNORM, // VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    m_BrightnessTexture = CreateImageTexture2D(
        "BrightnessRT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    for (auto &descriptorSets : mPlayerDescriptorSets) {
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorImageInfo imageInfo = {
                .sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = shadowMap.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL};

            vkutil::UpdateDescriptorSet(context, 2, imageInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
    }

    CreateFramebuffer();
}

void ForwardPass::BeginExecute(VkCommandBuffer cmd) {
    ZoneScopedN("ForwardPass::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "ForwardPass::BeginExecute", tracy::Color::Tomato);

 #ifdef _DEBUG
     vkutil::RenderPassLabel(cmd, "ForwardPass");
 #endif // !DEBUG

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    beginInfo.framebuffer = m_framebuffer;
    beginInfo.renderArea.extent = context.extent;

    VkClearValue clearValues[3];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    //clearValues[2].depthStencil = {1.0f, 0};
    clearValues[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    beginInfo.clearValueCount = 3;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    size_t playerCount = scene->GetActivePlayerCount();
    for (size_t playerId = 0; playerId < playerCount; ++playerId) {
        m_Skybox->Execute(cmd, playerCount, playerId);

        // NOTE: Viewport and scissor needs to be set again after executing skybox pass
        // TODO: Investigate more
        VkViewport viewport = CalcViewport(context.extent, playerCount, playerId);
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)};
        scissor.extent = {static_cast<uint32_t>(viewport.width),
                          static_cast<uint32_t>(viewport.height)};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaquePipeline.second, 0, 1,
                                &mPlayerDescriptorSets[playerId][vkutil::currentFrame], 0, nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skinnedPipeline.first);
        scene->DrawSkinned(cmd, m_skinnedPipeline.second);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaquePipeline.first);
        scene->DrawOpaque(cmd, m_opaquePipeline.second);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_alphaMaskPipeline.first);
        scene->DrawAlphaMasked(cmd, m_alphaMaskPipeline.second);
    }

    // Also drawn per player but per player logic is inside of the particle system
    ParticleSystem::DrawAll(cmd, m_particlePipeline.first, m_particlePipeline.second, mPlayerDescriptorSets);
}

void ForwardPass::EndExecute(VkCommandBuffer cmd) {
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "ForwardPass::EndExecute", tracy::Color::Tomato);

    vkCmdEndRenderPass(cmd);

 #ifdef _DEBUG
     vkutil::EndRenderPassLabel(cmd);
 #endif // !DEBUG
}

void ForwardPass::CreatePipeline() {

    std::vector<VkPushConstantRange> pushConstants = {

        {
         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
         .offset = 0,
         .size = sizeof(vkutil::MeshPushConstants)
        },
    };

    auto defaultPipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(OPAQUE_VERTEX_SHADER, ShaderType::VERTEX)
        .AddShader(OPAQUE_FRAGMENT_SHADER, ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({{meshDescriptorSetLayout, vkutil::materialDescriptorSetLayout}}, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_opaquePipeline = defaultPipelineResult;

    auto alphaMaskPipeline = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(ALPHA_MASK_VERTEX_SHADER, ShaderType::VERTEX)
        .AddShader(ALPHA_MASK_FRAGMENT_SHADER, ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({{meshDescriptorSetLayout, vkutil::materialDescriptorSetLayout}}, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_alphaMaskPipeline = alphaMaskPipeline;

    auto skinnedPipeline = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(SKINNED_VERTEX_SHADER, ShaderType::VERTEX)
        .AddShader(SKINNED_FRAGMENT_SHADER, ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout( {{meshDescriptorSetLayout, vkutil::materialDescriptorSetLayout, skinDescriptorSetLayout}}, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_skinnedPipeline = skinnedPipeline;

    m_particlePipeline = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(PARTICLE_MESH_VERTEX_SHADER, ShaderType::VERTEX)
        .AddShader(PARTICLE_SHADER_UNLIT_FRAGMENT, ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({meshDescriptorSetLayout, vkutil::materialDescriptorSetLayout, particleDescriptorSetLayout})
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

}

void ForwardPass::CreateRenderPass() {
    RenderPass builder(context.device, 1);
    //VK_FORMAT_B8G8R8A8_UNORM
    m_renderPass = builder
        .AddAttachment(context.swapchainFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .AddAttachment(VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .AddAttachment(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_NONE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        .AddColorAttachmentRef(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .AddColorAttachmentRef(0, 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .AddColorAttachmentRef(0, 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .SetDepthAttachmentRef(0, 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)

        // External -> 0 : Color
        .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // 0 -> External : Color : Wait for color writing to finish on the attachment before the fragment shader tries to read from it
        .AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // External -> 0 : Depth
        // Wait for the depth-prepass to finish writing to the depth attachment before this pass uses it for depth comparison
        .AddDependency(
            VK_SUBPASS_EXTERNAL, 0,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)

         // 0 -> External : Depth
         // Wait for this pass to finish reading from the depth attachment to occlude fragments before the depth-prepass writes to it
        .AddDependency(0, VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)

        // External -> 0 : Depth
        // External -> 0 : Depth
        //.AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)

        //// 0 -> External : Depth
        //.AddDependency(0, VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
        .Build();
}

void ForwardPass::CreateFramebuffer() {
    // Framebuffer
    std::vector<VkImageView> attachments = {m_RenderTarget.imageView, m_BrightnessTexture.imageView, m_NormalRoughness.imageView, depthPrepass.imageView };

    VkFramebufferCreateInfo fbcInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = context.extent.width,
        .height = context.extent.height,
        .layers = 1
    };

    VK_CHECK(vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_framebuffer), "Failed to create Forward pass framebuffer.");
}

void ForwardPass::BuildDescriptorSetLayouts() {
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), // CameraUBO (projection, view etc..)
        vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),                              // Light UBO
        vkutil::CreateDescriptorBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        vkutil::CreateDescriptorBinding(3, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
        vkutil::CreateDescriptorBinding(4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)

    };

    meshDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);

    skinDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}});
    particleDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}});
}

void ForwardPass::BuildDescriptors() {
    for (size_t playerId = 0; playerId < GlobalConfig::maxPlayers; ++playerId) {
        auto &descriptorSets = mPlayerDescriptorSets[playerId];

        descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, meshDescriptorSetLayout,
                                       vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);

        // Camera Transform UBO
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = scene->GetCameraBuffers(playerId)[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraTransform);
            vkutil::UpdateDescriptorSet(context, 0, bufferInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }

        // NOTE: At the moments the lights UBO and shadow map are not per player. However, with
        // light culling and cascaded shadow maps they will be per player

        // Light UBO
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = LightManager::getInstance().GetLightsUBO()[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(LightBuffer);
            vkutil::UpdateDescriptorSet(context, 1, bufferInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }

        // Shadow map descriptor
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorImageInfo imageInfo = {
                .sampler = vkutil::clampToEdgeSamplerAniso,
                .imageView = shadowMap.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL};

            vkutil::UpdateDescriptorSet(context, 2, imageInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = shadowMapRenderPass->GetCascadeUniformBuffer()[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(vkutil::CascadeMatrices);
            vkutil::UpdateDescriptorSet(context, 3, bufferInfo, descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_SHPass->GetSHBuffer().buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(vkutil::SHCoefficients);
            vkutil::UpdateDescriptorSet(context, 4, bufferInfo, descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        }
    }
}

void ForwardPass::Update() {
}
