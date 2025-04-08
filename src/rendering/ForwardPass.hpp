#pragma once
#include "Volk.hpp"
#include <memory>
#include <unordered_map>
#include "Camera.hpp"
#include "Skybox.hpp"

#include "Config.hpp"
#include "ShadowMap.hpp"


// This is disgusting whoever did it lol
#define SHADER_DIR std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/"
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
    ForwardPass(Context &context, Image &shadowMap, Image &depthPrepass, Scene *scene, const ShadowMap* shadowMapRenderPass);
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

    Image m_RenderTarget;
    Image m_DepthTarget;
    Image m_BrightnessTexture;
    Image m_NormalRoughness;

    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;
    VkDescriptorSetLayout meshDescriptorSetLayout;
    VkDescriptorSetLayout skinDescriptorSetLayout;
    VkDescriptorSetLayout particleDescriptorSetLayout;

    Context &context;
    Image &shadowMap;
    Image &depthPrepass;
    Scene *scene;
    const ShadowMap *shadowMapRenderPass;

    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> mPlayerDescriptorSets;

    std::pair<VkPipeline, VkPipelineLayout> m_opaquePipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_alphaMaskPipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_skinnedPipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_particlePipeline;

    std::unique_ptr<Skybox> m_Skybox;
};
