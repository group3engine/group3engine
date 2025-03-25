#pragma once
#include <vector>

#include "Image.hpp"
#include "Volk.hpp"
#include "Buffer.hpp"

class Context;

class Bloom {
  public:
    explicit Bloom(Context &context, Image &inputImage);
    ~Bloom();

    void Execute(VkCommandBuffer cmd) const;
    void Update();
    void Resize();

    Image &GetRenderTarget() { return m_BloomBlurYRT; }

  private:
    void CreatePipeline();
    void BuildHorizontalBlurDescriptors();
    void BuildVerticalBlurDescriptors();
    void CreateFramebuffer();
    void CreateRenderPass();

    void RenderHorizontalBlur(VkCommandBuffer cmd) const;
    void RenderVerticalBlur(VkCommandBuffer cmd) const;

    Context &context;
    Image m_BloomBlurXRT;
    Image m_BloomBlurYRT;
    Image &inputImage;

    VkRenderPass m_renderPass;
    VkFramebuffer m_HorizontalBlurFramebuffer;
    VkFramebuffer m_VerticalBlurFramebuffer;

    VkPipeline m_HorizontalBlurPipeline;
    VkPipelineLayout m_HorizontalBlurPipelineLayout;
    std::vector<VkDescriptorSet> m_HorizontalBlurDescriptorSets;
    VkDescriptorSetLayout m_HorizontalBlurDescriptorSetLayout;

    VkPipeline m_VerticalBlurPipeline;
    VkPipelineLayout m_VerticalBlurPipelineLayout;
    std::vector<VkDescriptorSet> m_VerticalBlurDescriptorSets;
    VkDescriptorSetLayout m_VerticalBlurDescriptorSetLayout;

    uint32_t m_width;
    uint32_t m_height;
};
