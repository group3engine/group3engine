#pragma once
#include "Volk.hpp"
#include <memory>
#include <unordered_map>
#include "Camera.hpp"

class Context;
class Scene;
class Buffer;

class ShadowMap {
  public:
    ShadowMap(Context &context, std::shared_ptr<Scene> &scene);
    ~ShadowMap();
    void Execute(VkCommandBuffer cmd) const;
    void Update();
    void Resize();

    Image &GetRenderTarget() { return m_ShadowMap; }

    void RebuildDescriptors();

  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();
    void DestroyDescriptors();

    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;

    Context &context;
    Image m_ShadowMap;
    uint32_t m_width;
    uint32_t m_height;

    std::shared_ptr<Scene> scene;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorSetLayout skinDescriptorSetLayout;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkPipeline m_SkinnedPipeline;
    VkPipelineLayout m_SkinnedPipelineLayout;
};
