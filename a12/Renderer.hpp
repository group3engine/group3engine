//
// Created by thomas on 29/01/25.
//

#ifndef VULKANTIME_RENDERER_HPP
#define VULKANTIME_RENDERER_HPP
#include <chrono>

#include "../labutils/UserState.hpp"
#include "../labutils/allocator.hpp"
#include "../labutils/angle.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vulkan_window.hpp"
#include "PerFrameResource.hpp"
#include "ShadowLight.hpp"
#include "ShadowLightManager.hpp"
#include "descriptorsets/descriptorsets.hpp"
#include "framebuffers/framebuffers.hpp"
#include "glsl.hpp"
#include "pipelines/pipeline.h"
#include "renderpasses/renderpasses.hpp"
#include "vulkan/vulkan.h"
#include "../gltf/MaterialManager.hpp"
#include "../gltf/MeshManager.hpp"
#include "../gltf/ResourceManager.hpp"

namespace lut = labutils;
using namespace labutils::literals;

using Clock_ = std::chrono::steady_clock;
using Secondsf_ = std::chrono::duration<float, std::ratio<1>>;

namespace GraphicsThings {

class Renderer {
   public:
    // constructor
    Renderer();

    // destructor
    ~Renderer();

    // render function, returns false if the window should close
    bool Render();

    ShadowLightManager *mShadowLightManager;

   private:
    VkPipeline m_ForwardPipeline;
    VkPipelineLayout m_ForwardPipelineLayout;

    VkPipeline m_BloomHorizontalPipeline;
    VkPipelineLayout m_BloomHorizontalPipelineLayout;

    VkPipeline m_BloomVerticalPipeline;
    VkPipelineLayout m_BloomVerticalPipelineLayout;

    VkPipeline m_MosiacPipeline;
    VkPipelineLayout m_MosiacPipelineLayout;

    // Not sure what this is being used for 
    VkPipeline m_PostProcessPipeline;
    VkPipelineLayout m_PostProcessPipelineLayout;


    lut::VulkanWindow mWindow;
    lut::Allocator mAllocator;
    lut::UserState mState{};
    PerFrameResource *mPerFrameResources = nullptr;
    lut::RenderPass mForwardRenderPass;
    lut::RenderPass mPostProcessRenderPass;
    lut::RenderPass mBloomRenderPass;
    lut::Image mRenderTexture;
    lut::ImageView mRenderTextureView;
    lut::Image mDepthBuffer;
    lut::ImageView mDepthBufferView;
    lut::Image mBloomBuffer;
    lut::ImageView mBloomBufferView;
    lut::Framebuffer mRenderTextureFramebuffer;
    lut::Framebuffer mBloomBufferFramebuffer;
    std::vector<glsl::UBO> mUbos;
    glsl::SceneUniform mSceneUniforms{};
    lut::Buffer mSceneUBO;
    lut::Buffer mLightingUBO;
    lut::Buffer mLightUBO;
    glm::mat4 mVPInverse{};
    lut::Buffer mVPInverseUBO;
    glm::vec4 mCameraPosition{};
    lut::Buffer mCameraPositionUBO;
    lut::DescriptorPool mStaticDescriptorPool;
    lut::DescriptorPool mDynamicDescriptorPool;
    lut::DescriptorSetLayout mSceneLayout;
    VkDescriptorSet mSceneDescriptors;
    lut::DescriptorSetLayout mLightingLayout;
    VkDescriptorSet mLightingDescriptors;
    lut::Sampler mDefaultSampler;
    lut::Sampler mPostProcessSampler;
    lut::DescriptorSetLayout mPostProcessLayout;
    VkDescriptorSet mPostProcessDescriptors;
    lut::DescriptorSetLayout mBloomLayout;
    VkDescriptorSet mBloomDescriptors;
    lut::Buffer mScreenSizeUBO;
    std::vector<lut::Image> mTextures;
    std::vector<lut::ImageView> mTextureViews;
    std::vector<VkDescriptorSet> mMaterialDescriptorSets;
    std::vector<StandardMesh> mAllMeshes;
    std::vector<StandardMesh const *> mAlphaMeshes;
    std::vector<StandardMesh const *> mOpaqueMeshes;
    lut::DescriptorSetLayout mMaterialLayout;
    lut::PipelineLayout mPipelineLayout;
    lut::PipelineLayout mPostProcessPipeLayout;
    lut::PipelineLayout mBloomPipeLayout;
    //PipelineBaseClass *mBasicPipeline;
    PipelineBaseClass *mAlphaPipeline;
    PipelineBaseClass *mPostProcessPipeline;
    PipelineBaseClass *mMosaicPipeline;
    PipelineBaseClass *mBloomFirstPassPipeline;
    PipelineBaseClass *mBloomSecondPassPipeline;


    MaterialManager *mMaterialManager;
    MeshManager *mMeshManager;
    bool mRecreateSwapchain = false;
    std::chrono::time_point<Clock_> mPreviousClock;
    const VkClearColorValue cClearColor = {{0.1f, 0.1f, 0.1f, 0.0f}};

    void RecreateSwapchain();
    void UpdateSceneUniforms();
    void UploadPerSceneUniforms(VkCommandBuffer aCmdBuff);
    void RecordForwardRenderPass(VkCommandBuffer aCmdBuff);
    void BeginRenderPass(VkCommandBuffer const &aCmdBuff,
                         VkRenderPass const &aRenderPass,
                         VkFramebuffer const &aFramebuffer,
                         VkExtent2D const &aImageExtent,
                         VkClearValue *clearValues, size_t numClearValues);
    void RecordPostProcessRenderPass(
        VkCommandBuffer aCmdBuff, VkRenderPass aPostProcessRenderPass,
        VkFramebuffer aSwapchainFramebuffer, VkExtent2D const &aImageExtent,
        VkPipeline const &aPPPipe, VkPipelineLayout aPostProcessPipelineLayout,
        std::vector<VkDescriptorSet> const &aPostProcessDescriptors);
    void PresentResults(std::uint32_t aImageIndex);
};

using Clock_ = std::chrono::steady_clock;
using Secondsf_ = std::chrono::duration<float, std::ratio<1>>;
namespace cfg {
// Compiled shader code for the graphics pipeline
// See sources in exercise4/shaders/*.
#define ASSETDIR_ "assets/a12/"
#define SHADERDIR_ ASSETDIR_ "shaders/"
constexpr char const *kPBRFragmentShaderPath = SHADERDIR_ "pbr.frag.spv";
constexpr char const *kPBRVertexShaderPath = SHADERDIR_ "pbr.vert.spv";
constexpr char const *kPBRFragmentShaderPathAlpha = SHADERDIR_ "pbra.frag.spv";
constexpr char const *kPostProcessVertexShaderPath =
    SHADERDIR_ "postprocess.vert.spv";
constexpr char const *kMosaicFragmentShaderPath = SHADERDIR_ "mosaic.frag.spv";
constexpr char const *kPPBloomFragmentShaderPath =
    SHADERDIR_ "blur_horizontal.frag.spv";
constexpr char const *kBloomVerticalFragmentShaderPath =
    SHADERDIR_ "blur_vertical.frag.spv";
constexpr char const *kPostProcessFragmentShaderPath =
    SHADERDIR_ "postprocessdefault.frag.spv";
#undef SHADERDIR_
constexpr char const *kBakedModelPath = ASSETDIR_ "suntemple.comp5892mesh";
#undef ASSETDIR_

// General rule: with a standard 24 bit or 32 bit float depth buffer,
// you can support a 1:1000 ratio between the near and far plane with
// minimal depth fighting. Larger ratios will introduce more depth
// fighting problems; smaller ratios will increase the depth buffer's
// resolution but will also limit the view distance.
constexpr float kCameraNear = 0.1f;
constexpr float kCameraFar = 100.f;

constexpr auto kCameraFov = 60.0_degf;

constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

constexpr VkFormat kNormalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat kAlbedoMetallicFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat kEmmisiveRoughnessFormat = VK_FORMAT_R8G8B8A8_UNORM;

constexpr int kMaxColourAttachments = 8;

// camera settings
lut::CameraSettings const kCameraSettings{};
}  // namespace cfg

}  // namespace GraphicsThings
#endif  // VULKANTIME_RENDERER_HPP
