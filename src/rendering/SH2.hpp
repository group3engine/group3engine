#pragma once
#include "Buffer.hpp"
#include "Camera.hpp"
#include "Config.hpp"
#include "Context.hpp"
#include "Image.hpp"
#include "Volk.hpp"

class Context;

class SH {

  public:
    explicit SH(Context &context, Scene *scene, Image &skybox);
    ~SH();
    void Execute(VkCommandBuffer cmd);
    const Buffer& GetSHBuffer() const { return m_SHUniform; }

  private:
    void CreatePipeline();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();

    Context &context;
    Scene *m_Scene;
    Image &skybox;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkDescriptorSetLayout mPlayerDescriptorSetLayout;
    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers>
        mPlayerDescriptorSets;

    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<VkDescriptorSet> mDescriptorSets;

    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;
    Buffer m_SHUniform;
};
