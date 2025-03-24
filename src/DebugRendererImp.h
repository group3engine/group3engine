// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

// See JoltPhysics/TestFramework/Renderer/DebugRendererImp.h

#ifndef DEBUGRENDERERIMP_H
#define DEBUGRENDERERIMP_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Mutex.h>

#ifdef JPH_DEBUG_RENDERER
// #include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif // JPH_DEBUG_RENDERER

#include <volk.h>

#include "Buffer.hpp"
#include "GLTFImportStructs.hpp"

class Renderer;

class DebugRendererImp final : public JPH::DebugRendererSimple {
  public:
    JPH_OVERRIDE_NEW_DELETE

    DebugRendererImp(Renderer *renderer);

    void Destroy();

    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;

    virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight) override;

    void Draw();

  private:
    void DrawLines();
    void DrawTriangles();

    void Clear();

  private:
    Renderer *mRenderer = nullptr;

    VkDescriptorSetLayout mUboLayout = VK_NULL_HANDLE;

    std::pair<VkPipeline, VkPipelineLayout> mLinePipeline = {nullptr, nullptr};

    std::vector<Buffer> mVertexBuffers;
    std::vector<VkDescriptorSet> mDescriptorSets;

    /// A single line segment
    struct Line {
        JPH::Float3                    mFrom;
        JPH::Color                     mFromColor;
        JPH::Float3                    mTo;
        JPH::Color                     mToColor;
    };

    /// The list of line segments
    JPH::Array<Line>                   mLines;
    JPH::Mutex                         mLinesLock;

    // Vertices for triangles
    JPH::Array<WPT::Vertex>            mVertices;
    JPH::Mutex                         mVerticesLock;
};
#endif // DEBUGRENDERERIMP_H
