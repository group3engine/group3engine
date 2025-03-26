#pragma once
#include "Volk.hpp"
#include <memory>
#include <unordered_map>
#include "Camera.hpp"
#include "Skybox.hpp"


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
    ForwardPass(Context &context, Image &shadowMap, Image &depthPrepass, std::shared_ptr<Scene> &scene, std::shared_ptr<Camera> &camera);
    ~ForwardPass();

    VkRenderPass Get() const { return m_renderPass; }

    void BeginExecute(VkCommandBuffer cmd) const;
    void EndExecute(VkCommandBuffer cmd) const;
    void Update();

    void Resize();
    Image &GetRenderTarget() { return m_RenderTarget; }
    Image &GetBrightnessTarget() { return m_BrightnessTexture; }
    Image &GetDepthTarget() { return m_DepthTarget; }

    Skybox* GetSkybox() { return m_Skybox.get(); }

    void RebuildDescriptors();

  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();
    void DestroyDescriptors();

    Image m_RenderTarget;
    Image m_DepthTarget;
    Image m_BrightnessTexture;

    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;
    VkDescriptorSetLayout meshDescriptorSetLayout;
    VkDescriptorSetLayout skinDescriptorSetLayout;

    Context &context;
    Image &shadowMap;
    Image &depthPrepass;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<Camera> camera;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::pair<VkPipeline, VkPipelineLayout> m_opaquePipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_alphaMaskPipeline;
    std::pair<VkPipeline, VkPipelineLayout> m_skinnedPipeline;

    std::unique_ptr<Skybox> m_Skybox;
};
