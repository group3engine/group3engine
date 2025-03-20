#include "ForwardPass.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Camera.hpp"
#include "Context.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"

ForwardPass::ForwardPass(Context &context, Image &shadowMap, Image &depthPrepass, std::shared_ptr<Scene> &scene, std::shared_ptr<Camera> &camera)
    : context{context},
      shadowMap{shadowMap},
      depthPrepass{depthPrepass},
      scene{scene},
      camera{camera} {

    m_RenderTarget = CreateImageTexture2D(
        "ForwardPassRT",
        context,
        context.extent.width,
        context.extent.height,
        context.swapchainFormat,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    m_DepthTarget = CreateImageTexture2D(
        "ForwardPassDepth",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1);

    m_BrightnessTexture = CreateImageTexture2D(
        "BrightnessRT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    BuildDescriptors();
    CreateRenderPass();
    CreateFramebuffer();
    CreatePipeline();

    m_Skybox = std::make_unique<Skybox>(context, camera, m_renderPass);
}

ForwardPass::~ForwardPass() {

    m_Skybox.reset();
    m_RenderTarget.Destroy(context.device);
    m_DepthTarget.Destroy(context.device);
    m_BrightnessTexture.Destroy(context.device);
    vkDestroyPipeline(context.device, m_opaquePipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_opaquePipeline.second, nullptr);
    vkDestroyPipeline(context.device, m_alphaMaskPipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_alphaMaskPipeline.second, nullptr);
    vkDestroyPipeline(context.device, m_skinnedPipeline.first, nullptr);
    vkDestroyPipelineLayout(context.device, m_skinnedPipeline.second, nullptr);

    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);
    vkDestroyRenderPass(context.device, m_renderPass, nullptr);
    if (meshDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context.device, meshDescriptorSetLayout, nullptr);
    }

    if(skinDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context.device, skinDescriptorSetLayout, nullptr);
    }
}

void ForwardPass::Resize() {

    uint32_t width = context.extent.width;
    uint32_t height = context.extent.height;

    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);

    m_RenderTarget.Destroy(context.device);
    m_DepthTarget.Destroy(context.device);
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

    m_DepthTarget = CreateImageTexture2D(
        "ForwardPassDepth",
        context,
        width,
        height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1);

    m_BrightnessTexture = CreateImageTexture2D(
        "BrightnessRT",
        context,
        context.extent.width,
        context.extent.height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = shadowMap.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(context, 2, imageInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    CreateFramebuffer();
}

void ForwardPass::Execute(VkCommandBuffer cmd) {
    ZoneScopedN("ForwardPass::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "ForwardPass", tracy::Color::Tomato);

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
    clearValues[2].depthStencil = {1.0f, 0};
    beginInfo.clearValueCount = 3;
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

    m_Skybox->Execute(cmd);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaquePipeline.second, 0, 1, &m_descriptorSets[vkutil::currentFrame], 0, nullptr);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skinnedPipeline.first);
    scene->DrawSkinned(cmd, m_skinnedPipeline.second);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaquePipeline.first);
    scene->DrawOpaque(cmd, m_opaquePipeline.second);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_alphaMaskPipeline.first);
    scene->DrawAlphaMasked(cmd, m_alphaMaskPipeline.second);


    vkCmdEndRenderPass(cmd);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG
}

void ForwardPass::CreatePipeline() {
    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(vkutil::MeshPushConstants)};

    // Default pipeline
    // .first  = VkPipeline
    // .second = VkPipelineLayout
    auto defaultPipelineResult = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(OPAQUE_VERTEX_SHADER, ShaderType::VERTEX)
        .AddShader(OPAQUE_FRAGMENT_SHADER, ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({{meshDescriptorSetLayout, vkutil::materialDescriptorSetLayout}}, pushConstantRange)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_opaquePipeline = defaultPipelineResult;

    auto alphaMaskPipeline = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(ALPHA_MASK_VERTEX_SHADER, ShaderType::VERTEX)
        .AddShader(ALPHA_MASK_FRAGMENT_SHADER, ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({{meshDescriptorSetLayout, vkutil::materialDescriptorSetLayout}}, pushConstantRange)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
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
        .SetPipelineLayout( {{meshDescriptorSetLayout, vkutil::materialDescriptorSetLayout, skinDescriptorSetLayout}}, pushConstantRange)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_skinnedPipeline = skinnedPipeline;
}

void ForwardPass::CreateRenderPass() {
    RenderPass builder(context.device, 1);

    m_renderPass = builder
        .AddAttachment(context.swapchainFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .AddAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .AddAttachment(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        .SetDepthAttachmentRef(0, 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        .AddColorAttachmentRef(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .AddColorAttachmentRef(0, 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        // External -> 0 : Color
        .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // 0 -> External : Color : Wait for color writing to finish on the attachment before the fragment shader tries to read from it
        .AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // External -> 0 : Depth
        // Wait for the depth-prepass to finish writing to the depth attachment before this pass uses it for depth comparison
        //.AddDependency(
        //	VK_SUBPASS_EXTERNAL, 0,
        //	VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        //	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        //	VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        //	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)

        // 0 -> External : Depth
        // Wait for this pass to finish reading from the depth attachment to occlude fragments before the depth-prepass writes to it
        //.AddDependency(0, VK_SUBPASS_EXTERNAL,
        //	VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        //	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        //	VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        //	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)

        // External -> 0 : Depth
        // External -> 0 : Depth
        .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)

        // 0 -> External : Depth
        .AddDependency(0, VK_SUBPASS_EXTERNAL,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
        .Build();
}

void ForwardPass::CreateFramebuffer() {
    // Framebuffer
    std::vector<VkImageView> attachments = {m_RenderTarget.imageView, m_BrightnessTexture.imageView, m_DepthTarget.imageView};
    VkFramebufferCreateInfo fbcInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = context.extent.width,
        .height = context.extent.height,
        .layers = 1};

    VK_CHECK(vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_framebuffer), "Failed to create Forward pass framebuffer.");
}

void ForwardPass::BuildDescriptors() {
    m_descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), // SceneUBO (projection, view etc..)
        vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),                              // Light UBO
        vkutil::CreateDescriptorBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};

    meshDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);

    vkutil::AllocateDescriptorSets(context, context.descriptorPool, meshDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, m_descriptorSets);
    skinDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}});

    // Camera Transform UBO
    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = camera->GetBuffers()[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CameraTransform);
        vkutil::UpdateDescriptorSet(context, 0, bufferInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    // Light UBO
    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = scene->GetLightsUBO()[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(vkutil::LightBuffer);
        vkutil::UpdateDescriptorSet(context, 1, bufferInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = shadowMap.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL};

        vkutil::UpdateDescriptorSet(context, 2, imageInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }
}

void ForwardPass::Update() {
}
