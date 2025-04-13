#pragma once
#include "Volk.hpp"
#include <memory>
#include "Image.hpp"
#include <vector>

class Context;
class Scene;

class Composite {
  public:
    explicit Composite(Context &context, Scene *scene, Image &LightingPass, Image &BloomPass, Image& SSAO, Image& SSRImage, Image& Fog, Image& Outline);
    ~Composite();

    void Execute(VkCommandBuffer cmd) const;
    void Update();
    void Resize();

    Image &GetRenderTarget() { return m_RenderTarget; }

  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptors();

    Context &context;
    Scene *m_Scene;
    Image m_RenderTarget;
    Image &LightingPass;
    Image &BloomPass;
    Image &SSAO;
    Image &SSRImage;
    Image &Fog;
    Image &Outline;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;

    uint32_t m_width;
    uint32_t m_height;
};
