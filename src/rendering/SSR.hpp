#pragma once
#include "Volk.hpp"
#include "Image.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"

class Context;

class SSR {
  public:

    explicit SSR(Context &context, Scene *scene, Image& depthBuffer, Image& renderedScene, Image& metallicRoughness, Image& skybox);
    ~SSR();
    void Execute(VkCommandBuffer cmd) const;
    void Update();
    void Resize();

    Image& GetRenderTarget() { return m_RenderTarget; }

  private:
    void CreatePipeline();
    void BuildDescriptors();
    void CreateFramebuffer();
    void CreateRenderPass();

    Context &context;
    Scene *m_Scene;
    uint32_t m_width;
    uint32_t m_height;
    Image m_RenderTarget;
    Image& depthBuffer;
    Image& renderedScene;
    Image& metallicRoughness;
    Image& skybox;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;
    std::vector<Buffer> m_SSRUniform;
};
