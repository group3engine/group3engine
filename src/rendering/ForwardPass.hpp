#pragma once
#include "Volk.hpp"
#include <memory>
#include <unordered_map>
#include "Camera.hpp"
#include "Skybox.hpp"

#include "Config.hpp"
#include "ShadowMap.hpp"
#include "SH2.hpp"
#include "PrefilterSkybox.hpp"
#include "IrradianceMap.hpp"

// This is disgusting whoever did it lol
#define SHADER_DIR assetsPath / "shaders/"
#define OPAQUE_FRAGMENT_SHADER SHADER_DIR / "default.frag.spv"
#define OPAQUE_VERTEX_SHADER SHADER_DIR / "default.vert.spv"
#define ALPHA_MASK_FRAGMENT_SHADER SHADER_DIR / "alpha_masking.frag.spv"
#define ALPHA_MASK_VERTEX_SHADER SHADER_DIR / "default.vert.spv"
#define SKINNED_FRAGMENT_SHADER SHADER_DIR / "default.frag.spv"
#define SKINNED_VERTEX_SHADER SHADER_DIR / "skinned.vert.spv"

class Context;
class Scene;
class Buffer;


class ForwardPass {

  public:
    ForwardPass(Context &context, const Image &shadowMap, Image &depthPrepass, Scene *scene, const ShadowMap* shadowMapRenderPass, const std::vector<Buffer>& debugUniform);
    ~ForwardPass();

    VkRenderPass Get() const { return m_renderPass; }

    void BeginExecute(VkCommandBuffer cmd);
    void EndExecute(VkCommandBuffer cmd);
    void Update();

    void Resize();
    Image &GetRenderTarget() { return m_RenderTarget; }
    Image &GetBrightnessTarget() { return m_BrightnessTexture; }
    Image &GetDepthTarget() { return m_DepthTarget; }
    Image &GetNormalRoughnessTarget() { return m_NormalRoughness; }
    Skybox* GetSkybox() { return m_Skybox.get(); }


  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();

    Image m_RenderTarget; // Render 4x to this one
    Image m_SingleSampleRenderTarget; // 1x render target

    Image m_DepthTarget;
    Image m_BrightnessTexture;
    Image m_NormalRoughness;

    Image m_LavaFlowMap;

    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;
    VkDescriptorSetLayout meshDescriptorSetLayout;
    VkDescriptorSetLayout skinDescriptorSetLayout;
    VkDescriptorSetLayout particleDescriptorSetLayout;
    VkDescriptorSetLayout lavaFlowMapDescriptorSetLayout;

    Context &context;
    const std::vector<Buffer>& m_DebugUniform;
    const Image &shadowMap;
    Image &depthPrepass;
    Scene *scene;
    const ShadowMap *shadowMapRenderPass;

    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> mPlayerDescriptorSets;
    VkDescriptorSet mLavaFlowMapDescriptorSet;

    std::pair<VkPipeline, VkPipelineLayout> m_opaquePipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_alphaMaskPipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_skinnedPipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_particlePipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_wireframePipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_skinnedWireframePipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_lavaPipeline;

    std::unique_ptr<Skybox> m_Skybox;
    std::unique_ptr<PrefilterSkybox> PrefilteredSkybox;
    std::unique_ptr<SH> m_SHPass;
    std::unique_ptr<IrradianceMap> m_IrradianceMap;

    VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_4_BIT;
};
