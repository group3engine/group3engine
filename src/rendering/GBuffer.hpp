#pragma once

#include "Volk.hpp"
#include <memory>
#include <unordered_map>
#include "Camera.hpp"

/*
*	This is a thin g-buffer required for post-processing such as SSAO and SSR
*/

class Context;
class Scene;
class Buffer;

class GBuffer {
  public:

    GBuffer(Context &context, std::shared_ptr<Scene> &scene, std::shared_ptr<Camera> &camera);
    ~GBuffer();
    void Execute(VkCommandBuffer cmd) const;
    void Update();

    void Resize();

    Image& GetMetallicRoughnessTarget() { return m_RenderTarget; }

  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();

    uint32_t m_width;
    uint32_t m_height;

    Image m_RenderTarget;
    Image m_DepthTarget;
    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;

    Context &context;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<Camera> camera;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkPipeline m_AlphaMaskingPipeline;
    VkPipelineLayout m_AlphaMaskingPipelineLayout;
};
