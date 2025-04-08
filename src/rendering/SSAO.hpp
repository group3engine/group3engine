#pragma once
#include "Volk.hpp"
#include "Image.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"

#include "Config.hpp"

/* SSAO needs to be blurred
    currently the composite pass is taking a non-blurred version which may appear a bit noisy
    in the final image
*/

class Context;

class SSAO {
  public:

    explicit SSAO(Context &context, Scene *scene, Image& depthBuffer, Image& normalRoughnessImage);
    ~SSAO();
    void Execute(VkCommandBuffer cmd);
    void Update();
    void Resize();

    Image& GetRenderTarget() { return m_RenderTarget; }

  private:
    void CreatePipeline();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();
    void CreateFramebuffer();
    void CreateRenderPass();
    void GenerateNoiseTexture(uint32_t width, uint32_t height);

    Context &context;
    Scene *m_Scene;
    uint32_t m_width;
    uint32_t m_height;
    Image m_RenderTarget;
    Image& depthBuffer;
    Image& normalRoughnessImage;
    Image m_NoiseTexture;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkDescriptorSetLayout mPlayerDescriptorSetLayout;
    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> mPlayerDescriptorSets;

    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<VkDescriptorSet> mDescriptorSets;

    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;
    std::vector<Buffer> m_SSAOUniform;
};
