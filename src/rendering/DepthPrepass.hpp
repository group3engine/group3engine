#pragma once

#include <memory>
#include <vector>

#include "Image.hpp"
#include "Volk.hpp"

class Context;
class Scene;
class Image;
class Camera;

class DepthPrepass {
  public:
    explicit DepthPrepass(Context &context, std::shared_ptr<Scene> scene, std::shared_ptr<Camera> camera);
    ~DepthPrepass();

    void Execute(VkCommandBuffer cmd) const;
    void Resize();

    Image &GetRenderTarget() { return m_DepthTarget; };

  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();

    Context &context;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<Camera> camera;
    Image m_DepthTarget;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;

    uint32_t m_width;
    uint32_t m_height;
};
