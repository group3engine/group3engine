#pragma once

#include <memory>
#include <array>
#include <vector>

#include "Image.hpp"
#include "Volk.hpp"

#include "Config.hpp"

class Context;
class Scene;
class Image;

class DepthPrepass {
  public:
    explicit DepthPrepass(Context &context, Scene *scene);
    ~DepthPrepass();

    void Execute(VkCommandBuffer cmd);
    void Resize();

    Image &GetRenderTarget() { return m_DepthTarget; };

  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();

    Context &context;
    Scene *m_Scene;
    Image m_DepthTarget;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkDescriptorSetLayout mPlayerDescriptorSetLayout;
    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> mPlayerDescriptorSets;

    // Create descriptor sets that are not per player here if needed
    // std::vector<VkDescriptorSet> mDescriptorSets;

    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;

    uint32_t m_width;
    uint32_t m_height;
};
