#pragma once
#include "Volk.hpp"
#include "Image.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"

/* SSAO needs to be blurred
    currently the composite pass is taking a non-blurred version which may appear a bit noisy
    in the final image
*/

class Context;

class SSAO {
  public:

    struct SSAOSamples
    {
        glm::vec3 samples[64];
    };

    explicit SSAO(Context &context, Scene *scene, Image& depthBuffer, Image& renderedScene);
    ~SSAO();
    void Execute(VkCommandBuffer cmd) const;
    void Update();
    void Resize();

    Image& GetRenderTarget() { return m_RenderTarget; }

  private:
    void CreatePipeline();
    void BuildDescriptors();
    void CreateFramebuffer();
    void CreateRenderPass();

    void GenerateSSAOSamples();
    void GenerateNoiseTexture(uint32_t width, uint32_t height);

    float lerp(float a, float b, float f)
    {
        return a + f * (b - a);
    }

    Context &context;
    Scene *m_Scene;
    uint32_t m_width;
    uint32_t m_height;
    Image m_RenderTarget;
    Image& depthBuffer;
    Image &renderedScene;
    Image m_NoiseTexture;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;
    std::vector<Buffer> m_SSAOUniform;
    Buffer m_SSAOSamples;
    SSAOSamples ssaoSamplesCPUtoGPU;
};
