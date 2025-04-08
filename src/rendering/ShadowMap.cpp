#include "ShadowMap.hpp"

#include <tracy/TracyVulkan.hpp>

#include "Camera.hpp"
#include "Context.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"

#include "RenderPassCommon.hpp"

#define RESOLUTION 2048

ShadowMap::ShadowMap(Context &context, Scene *scene)
    : context{context}, scene{scene} {

    // 128, 256, 512, 1024, 2048, 4096
    m_width = RESOLUTION;
    m_height = RESOLUTION;

    m_ShadowMap = CreateImageTexture2D(
        "ShadowMap_Depth_RT",
        context,
        m_width,
        m_height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1,
        0U,
        NUM_SHADOW_CASCADES // This imaege has N number of cascades
    );

    // Create the cascade uniform buffer to send cascade matrices to GPU
    m_CascadeUniformBuffer.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    for (auto& buffer : m_CascadeUniformBuffer)
    {
        buffer = CreateBuffer("CascadeUB", context, sizeof(vkutil::CascadeMatrices), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    //m_CascadeImageViews.resize(NUM_SHADOW_CASCADES);
    //m_CascadeFramebuffer.resize(NUM_SHADOW_CASCADES);
    m_Cascades.resize(NUM_SHADOW_CASCADES);

    BuildDescriptorSetLayouts();
    BuildDescriptors();
    CreateRenderPass();
    CreateFramebuffer();
    CreatePipeline();

    // Create the image views and framebuffers for each cascade
    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.image = m_ShadowMap.image;
        viewInfo.subresourceRange = {};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = i; // first layer of the image that this image view will use
        viewInfo.subresourceRange.layerCount = 1; // Access to only its own layer
        VK_CHECK(vkCreateImageView(context.device, &viewInfo, nullptr, &m_Cascades[i].imgView), "Failed to create image view for cascade: " + i);

        // Create the framebuffer
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &m_Cascades[i].imgView;
        fbInfo.width = m_width;
        fbInfo.height = m_height;
        fbInfo.layers = 1;
        VK_CHECK(vkCreateFramebuffer(context.device, &fbInfo, nullptr, &m_Cascades[i].framebuffer), "Failed to create framebuffer for cascade: " + i);
    }
}

ShadowMap::~ShadowMap() {

    m_ShadowMap.Destroy(context.device);
    vkDestroyPipeline(context.device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);

    // Destroy cascade resources
    for (auto& cascade : m_Cascades)
    {
        cascade.Destroy(context.device);
    }

    // Destroy cascade uniform buffer
    for (auto& buffer : m_CascadeUniformBuffer)
    {
        buffer.Destroy();
    }

    vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);
    vkDestroyRenderPass(context.device, m_renderPass, nullptr);
    vkDestroyDescriptorSetLayout(context.device, mPlayerDescriptorSetLayout, nullptr);

    vkDestroyPipeline(context.device, m_SkinnedPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_SkinnedPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, skinDescriptorSetLayout, nullptr);
}

void ShadowMap::Execute(VkCommandBuffer cmd) {
    ZoneScopedN("ShadowMap::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "ShadowMap", tracy::Color::DimGrey);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "ShadowMap");
#endif

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_renderPass;
    //beginInfo.framebuffer = m_framebuffer;
    beginInfo.renderArea.extent = {m_width, m_height};

    VkClearValue clearValues[1];
    clearValues[0].depthStencil.depth = {1.0f};
    beginInfo.clearValueCount = 1;
    beginInfo.pClearValues = clearValues;

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_width;
    viewport.height = m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0,0};
    scissor.extent.width = m_width;
    scissor.extent.height = m_height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &mPlayerDescriptorSets[0][vkutil::currentFrame], 0, nullptr);
    vkCmdSetDepthBias(cmd, vkutil::ShadowBias, 0.0f, vkutil::ShadowSlope);

    // For each cascade, render the scene
    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
#ifdef _DEBUG
        vkutil::RenderPassLabel(cmd, ("Cascade: " + std::to_string(i)).c_str());
#endif
        beginInfo.framebuffer = m_Cascades[i].framebuffer; // Bind framebuffer for current cascade
        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Pass the current index down ? not best solution but could work
        const uint32_t cascadeIndex = i;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkinnedPipeline);
        scene->DrawSkinned(cmd, m_SkinnedPipelineLayout, cascadeIndex);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        scene->DrawShadowMap(cmd, m_PipelineLayout, cascadeIndex);
        vkCmdEndRenderPass(cmd);
#ifdef _DEBUG
        vkutil::EndRenderPassLabel(cmd);
#endif
    }
}

void ShadowMap::CreatePipeline() {

    std::vector<VkPushConstantRange> pushConstants = {

        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(vkutil::MeshPushConstants)
        }
    };

    // Default pipeline
    auto ShadowMapPipelineRes = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "shadow_map.vert.spv", ShaderType::VERTEX)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "shadow_map.frag.spv", ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE)
        .SetPipelineLayout({{mPlayerDescriptorSetLayout, vkutil::materialDescriptorSetLayout}}, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_Pipeline = ShadowMapPipelineRes.first;
    m_PipelineLayout = ShadowMapPipelineRes.second;

    auto skinnedShadowMapPipelineRes = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "shadow_map_skinned.vert.spv", ShaderType::VERTEX)
        .AddShader(std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/" / "shadow_map.frag.spv", ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS}})
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_TRUE)
        .SetPipelineLayout({{mPlayerDescriptorSetLayout, vkutil::materialDescriptorSetLayout, skinDescriptorSetLayout}}, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_renderPass)
        .Build();

    m_SkinnedPipeline = skinnedShadowMapPipelineRes.first;
    m_SkinnedPipelineLayout = skinnedShadowMapPipelineRes.second;

}

void ShadowMap::CreateRenderPass() {
    RenderPass builder(context.device, 1);

    m_renderPass = builder
        .AddAttachment(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL)
        .SetDepthAttachmentRef(0, 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)

        // External -> 0 : Depth ( Wait for previous depth writes to finish before writing )
        .AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        // 0 -> External : Depth ( Wait for depth to finish writing in this render pass before allowing other render passes to try and sample from it )
        .AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT)

        .Build();
}

void ShadowMap::CreateFramebuffer() {
    // Framebuffer
    std::vector<VkImageView> attachments = { m_ShadowMap.imageView };

    VkFramebufferCreateInfo fbcInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = m_renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = m_width,
        .height = m_height,
        .layers = 1};

    VK_CHECK(vkCreateFramebuffer(context.device, &fbcInfo, nullptr, &m_framebuffer), "Failed to create Forward pass framebuffer.");
}

void ShadowMap::BuildDescriptorSetLayouts() {
    // Light UBO
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        vkutil::CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
    };

    mPlayerDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);

    skinDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}});
}

void ShadowMap::BuildDescriptors() {
    for (auto &descriptorSets : mPlayerDescriptorSets) {
        descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mPlayerDescriptorSetLayout,
                                       vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);

        // Lights UBO
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = LightManager::getInstance().GetLightsUBO()[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(vkutil::LightUBO) * LightManager::getInstance().GetLights().size();
            vkutil::UpdateDescriptorSet(context, 0, bufferInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }

        // Cascade uniform to use cascade matrices in shader
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_CascadeUniformBuffer[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(m_cascadeMatricesData);
            vkutil::UpdateDescriptorSet(context, 1, bufferInfo, descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }
}

void ShadowMap::Update() {

    float cascadeSplits[NUM_SHADOW_CASCADES];

    const auto &playerCameraTransform = scene->GetPlayerCameraTransforms()[0];
    float nearClip = playerCameraTransform.nearPlane;
    float farClip = playerCameraTransform.farPlane;

    float clipRange = farClip - nearClip;

    float minZ = nearClip;
    float maxZ = nearClip + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
        float p = (i + 1) / static_cast<float>(NUM_SHADOW_CASCADES);
        float log = minZ * std::pow(ratio, p);
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearClip) / clipRange;
    }

    // Calculate orthographic projection matrix for each cascade
    float lastSplitDist = 0.0;
    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
        float splitDist = cascadeSplits[i];

        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, -1.0f, 1.0f),
        };

        // Project frustum corners into world space
        glm::mat4 invCam = glm::inverse(playerCameraTransform.projection * playerCameraTransform.view);
        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
            frustumCorners[j] = invCorner / invCorner.w;
        }

        for (uint32_t j = 0; j < 4; j++) {
            glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
            frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
            frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
        }

        // Get frustum center
        glm::vec3 frustumCenter = glm::vec3(0.0f);
        for (uint32_t j = 0; j < 8; j++) {
            frustumCenter += frustumCorners[j];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;
        for (uint32_t j = 0; j < 8; j++) {
            float distance = glm::length(frustumCorners[j] - frustumCenter);
            radius = glm::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3(radius);
        glm::vec3 minExtents = -maxExtents;

        float angle = glm::radians(glfwGetTime() * 360.0f);
        float r = 20.0f;
        //glm::vec3 lightPos = glm::vec3(cos(angle) * r, -r, sin(angle) * r);
        //scene->GetLights()[0].position = glm::vec4(lightPos, 1.0f);
        glm::vec3 lightDir = glm::normalize(LightManager::getInstance().GetLights()[0]->position);
        glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -200.0f, maxExtents.z - minExtents.z);

        // Store split distance and matrix in cascade
        m_Cascades[i].splitDepth = (nearClip + splitDist * clipRange) * -1.0f;
        m_Cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

        lastSplitDist = cascadeSplits[i];
    }

    // Update the uniform buffer
    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        m_cascadeMatricesData.matrices[i] = m_Cascades[i].viewProjMatrix;
    }

    m_cascadeMatricesData.cascadeSplits.x = m_Cascades[0].splitDepth;
    m_cascadeMatricesData.cascadeSplits.y = m_Cascades[1].splitDepth;
    m_cascadeMatricesData.cascadeSplits.z = m_Cascades[2].splitDepth;
    m_cascadeMatricesData.cascadeSplits.w = m_Cascades[3].splitDepth;

    m_CascadeUniformBuffer[vkutil::currentFrame].WriteToBuffer(m_cascadeMatricesData, sizeof(m_cascadeMatricesData));
}
