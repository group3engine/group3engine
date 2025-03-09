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

    explicit SSAO(Context &context, Image& depthBuffer, Image& renderedScene, std::shared_ptr<Camera> camera);
    ~SSAO();
    void Execute(VkCommandBuffer cmd);
    void Update();
    void Resize();

    Image& GetRenderTarget() { return m_RenderTarget; }

  private:
    void CreatePipeline();
    void BuildDescriptors();
    void CreateFramebuffer();
    void CreateRenderPass();

    Context &context;
    uint32_t m_width;
    uint32_t m_height;
    Image m_RenderTarget;
    Image& depthBuffer;
    Image &renderedScene;
    std::shared_ptr<Camera> camera;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;
    std::vector<Buffer> m_SSAOUniform;
};
