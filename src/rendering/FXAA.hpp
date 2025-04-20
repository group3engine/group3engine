#pragma once
#include "Buffer.hpp"
#include "Camera.hpp"
#include "Image.hpp"
#include "Volk.hpp"

#include "Config.hpp"

class Context;

class FXAA {
  public:
    explicit FXAA(Context &context, Image& compositeImage);
    ~FXAA();
    void Execute(VkCommandBuffer cmd);
    void Update();
    void Resize();

    Image &GetRenderTarget() { return m_RenderTarget; }

  private:
    void CreatePipeline();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();
    void CreateFramebuffer();
    void CreateRenderPass();

    Context &context;
    Scene *m_Scene;
    Image m_RenderTarget;
    Image &compositeImage;
    uint32_t m_width;
    uint32_t m_height;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<VkDescriptorSet> mDescriptorSets;

    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;
    std::vector<Buffer> m_FXAAUniform;
};
