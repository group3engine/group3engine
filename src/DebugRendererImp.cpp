// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

// See JoltPhysics/TestFramework/Renderer/DebugRendererImp.cpp
#ifdef JPH_DEBUG_RENDERER

#include "DebugRendererImp.h"


#include <spdlog/spdlog.h>

#include "Buffer.hpp"
#include "Pipeline.hpp"
#include "Renderer.hpp"
#include "RenderPassCommon.hpp"

namespace {
    #define SHADER_DIR assetsPath / "shaders/"
    #define LINE_VERTEX_SHADER SHADER_DIR / "line.vert.spv"
    #define LINE_FRAGMENT_SHADER SHADER_DIR / "line.frag.spv"
    #define TRIANGLE_VERTEX_SHADER SHADER_DIR / "triangle.vert.spv"
    #define TRIANGLE_FRAGMENT_SHADER SHADER_DIR / "triangle.frag.spv"
}

DebugRendererImp::DebugRendererImp(Renderer *renderer, Scene *scene)
    : mRenderer(renderer), mScene(scene) {
    mLineVertexBuffers.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    mVertexBuffers.resize(vkutil::MAX_FRAMES_IN_FLIGHT);

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        // CameraUBO (projection, view etc.)
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)};

    auto &context = mRenderer->GetContext();

    mUboLayout = vkutil::CreateDescriptorSetLayout(context, bindings);

    for (size_t playerId = 0; playerId < GlobalConfig::maxPlayers; ++playerId) {
        auto &descriptorSets = mDescriptorSets[playerId];

        descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mUboLayout,
                                        vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);

        // Camera UBO
        for (size_t i = 0; i < std::size_t(vkutil::MAX_FRAMES_IN_FLIGHT); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mScene->GetCameraBuffers(playerId)[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraTransform);
            vkutil::UpdateDescriptorSet(context, 0, bufferInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }

    // TODO: Fix vertex attribute description to work with Line struct
    {
        VkVertexInputBindingDescription bindingDescription{
            .binding = 0,
            .stride = offsetof(Line, mTo),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        std::vector<VkVertexInputAttributeDescription> attributeDescription = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Line, mFrom)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .offset = offsetof(Line, mFromColor)
            }
        };

        mLinePipeline = PipelineBuilder(mRenderer->GetContext().device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
            .AddShader(LINE_VERTEX_SHADER, ShaderType::VERTEX)
            .AddShader(LINE_FRAGMENT_SHADER, ShaderType::FRAGMENT)
            .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
            .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
            .SetRasterizationState(VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetPipelineLayout({mUboLayout})
            .SetSampling(VK_SAMPLE_COUNT_1_BIT)
            .AddBlendAttachmentState()
            .AddBlendAttachmentState()
            .AddBlendAttachmentState()
            .SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetRenderPass(mRenderer->GetForwardPass()->Get())
            .OverrideBindingDescription(bindingDescription)
            .OverrideAttributeDescription(attributeDescription)
            .Build();
    }

    // TODO: Update vertex attribute description to work with a Triangle struct.
    // The Vertex struct used elsewhere is larger than needed here.
    {
        VkVertexInputBindingDescription bindingDescription{
            .binding = 0,
            .stride = sizeof(TriangleVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        std::vector<VkVertexInputAttributeDescription> attributeDescription = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(TriangleVertex, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .offset = offsetof(TriangleVertex, color)
            }
        };

        // NOTE: Wireframe triangles
        mTrianglePipeline = PipelineBuilder(mRenderer->GetContext().device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
            .AddShader(TRIANGLE_VERTEX_SHADER, ShaderType::VERTEX)
            .AddShader(TRIANGLE_FRAGMENT_SHADER, ShaderType::FRAGMENT)
            .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .SetDynamicState({{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
            .SetRasterizationState(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetPipelineLayout({mUboLayout})
            .SetSampling(VK_SAMPLE_COUNT_1_BIT)
            .AddBlendAttachmentState()
            .AddBlendAttachmentState()
            .AddBlendAttachmentState()
            .SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetRenderPass(mRenderer->GetForwardPass()->Get())
            .OverrideBindingDescription(bindingDescription)
            .OverrideAttributeDescription(attributeDescription)
            .Build();
    }
}

void DebugRendererImp::Destroy() {
    vkDeviceWaitIdle(mRenderer->GetContext().device);

    vkDestroyPipeline(mRenderer->GetContext().device, mLinePipeline.first, nullptr);
    vkDestroyPipelineLayout(mRenderer->GetContext().device, mLinePipeline.second, nullptr);

    vkDestroyPipeline(mRenderer->GetContext().device, mTrianglePipeline.first, nullptr);
    vkDestroyPipelineLayout(mRenderer->GetContext().device, mTrianglePipeline.second, nullptr);

    vkDestroyDescriptorSetLayout(mRenderer->GetContext().device, mUboLayout, nullptr);
}

void DebugRendererImp::DrawLine(RVec3Arg inFrom, RVec3Arg inTo, ColorArg inColor)
{
    ZoneScopedN("DebugRendererImp::DrawLine");

    Line line;
    Vec3(inFrom).StoreFloat3(&line.mFrom);
    line.mFromColor = inColor;
    Vec3(inTo).StoreFloat3(&line.mTo);
    line.mToColor = inColor;

    lock_guard lock(mLinesLock);
    mLines.push_back(line);
}

void DebugRendererImp::DrawTriangle(RVec3Arg inV1, RVec3Arg inV2, RVec3Arg inV3, ColorArg inColor, ECastShadow inCastShadow)
{
    ZoneScopedN("DebugRendererImp::DrawTriangle");

    lock_guard lock(mVerticesLock);

    // Construct triangle
    TriangleVertex v1{glm::vec3{inV1.GetX(), inV1.GetY(), inV1.GetZ()}, inColor};
    TriangleVertex v2{glm::vec3{inV2.GetX(), inV2.GetY(), inV2.GetZ()}, inColor};
    TriangleVertex v3{glm::vec3{inV3.GetX(), inV3.GetY(), inV3.GetZ()}, inColor};
    mVertices.push_back(v1);
    mVertices.push_back(v2);
    mVertices.push_back(v3);
}

void DebugRendererImp::DrawText3D(RVec3Arg inPosition, const std::string_view &inString, ColorArg inColor, float inHeight) {
    spdlog::info("DrawText3D");
}

void DebugRendererImp::DrawLines() {
    ZoneScopedN("DebugRendererImp::DrawLines");

    lock_guard lock(mLinesLock);

    // Draw the lines
    if (!mLines.empty()) {
        // Create line primitive with renderer
        VkCommandBuffer commandBuffer = mRenderer->GetPrimaryCommandBuffer();

        // Create vertex buffer
        VkDeviceSize size = sizeof(Line) * mLines.size();
        CreateAndUploadBuffer(mRenderer->GetContext(), mLines.data(), size,
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              mLineVertexBuffers[vkutil::currentFrame]);

        // Draw
        VkBuffer vertex_buffers[] = {mLineVertexBuffers[vkutil::currentFrame].buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertex_buffers, offsets);

        VkPipeline linePipeline = mLinePipeline.first;
        VkPipelineLayout linePipelineLayout = mLinePipeline.second;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);

        size_t playerCount = mScene->GetActivePlayerCount();
        for (size_t playerId = 0; playerId < playerCount; ++playerId) {
            auto &descriptorSets = mDescriptorSets[playerId];

            VkViewport viewport =
                CalcViewport(mRenderer->GetContext().extent, playerCount, playerId);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)};
            scissor.extent = {static_cast<uint32_t>(viewport.width),
                              static_cast<uint32_t>(viewport.height)};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    linePipelineLayout, 0, 1, &descriptorSets[vkutil::currentFrame],
                                    0, nullptr);

            vkCmdDraw(commandBuffer, mLines.size() * 2, 1, 0, 0);
        }
    }

    mLines.clear();

    // Push the vertex buffer to be freed the next time this frame is used. Do
    // this to delay the vertex buffer being freed so it can be used this frame.
    auto &vtxBuff = mLineVertexBuffers[vkutil::currentFrame];
    FreedBuffer freedBuffer = {vtxBuff.buffer, vtxBuff.allocation, vtxBuff.allocator};
    mRenderer->mFreedBuffers[vkutil::currentFrame].push_back(freedBuffer);
}

void DebugRendererImp::DrawTriangles() {
    ZoneScopedN("DebugRendererImp::DrawTriangles");

    // NOTE: Draws wireframe triangles with the current pipeline

    lock_guard lock(mVerticesLock);

    // Draw the lines
    if (!mVertices.empty()) {
        // Create line primitive with renderer
        VkCommandBuffer commandBuffer = mRenderer->GetPrimaryCommandBuffer();

        // Create vertex buffer
        VkDeviceSize size = sizeof(TriangleVertex) * mVertices.size();
        CreateAndUploadBuffer(mRenderer->GetContext(), mVertices.data(), size,
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              mVertexBuffers[vkutil::currentFrame]);

        // Draw
        VkBuffer vertex_buffers[] = {mVertexBuffers[vkutil::currentFrame].buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertex_buffers, offsets);

        VkPipeline trianglePipeline = mTrianglePipeline.first;
        VkPipelineLayout trianglePipelineLayout = mTrianglePipeline.second;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);

        size_t playerCount = mScene->GetActivePlayerCount();
        for (size_t playerId = 0; playerId < playerCount; ++playerId) {
            auto &descriptorSets = mDescriptorSets[playerId];

            VkViewport viewport =
                CalcViewport(mRenderer->GetContext().extent, playerCount, playerId);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)};
            scissor.extent = {static_cast<uint32_t>(viewport.width),
                              static_cast<uint32_t>(viewport.height)};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    trianglePipelineLayout, 0, 1, &descriptorSets[vkutil::currentFrame],
                                    0, nullptr);

            vkCmdDraw(commandBuffer, mVertices.size(), 1, 0, 0);
        }
    }

    mVertices.clear();

    // Push the vertex buffer to be freed the next time this frame is used. Do
    // this to delay the vertex buffer being freed so it can be used this frame.
    auto &vtxBuff = mVertexBuffers[vkutil::currentFrame];
    FreedBuffer freedBuffer = {vtxBuff.buffer, vtxBuff.allocation, vtxBuff.allocator};
    mRenderer->mFreedBuffers[vkutil::currentFrame].push_back(freedBuffer);
}

void DebugRendererImp::Draw() {
    ZoneScopedN("DebugRendererImp::Draw");
    TracyVkZoneC(mRenderer->GetContext().tracyContexts[vkutil::currentFrame],
                 mRenderer->GetPrimaryCommandBuffer(), "DebugRenderer::Draw",
                 tracy::Color::Coral4);

    DrawLines();
    DrawTriangles();
}

#endif