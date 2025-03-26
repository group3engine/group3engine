#pragma once
#include "Volk.hpp"
#include <memory>
#include "Camera.hpp"

class Context;
class Scene;
class Buffer;

class PresentPass {
  public:
    PresentPass(Context &context, Scene *scene, Image &renderedScene);
    ~PresentPass();
    void Execute(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void Update();
    void Resize();

  private:
    void CreatePipeline();
    void BuildDescriptors();

    Context &context;
    Scene *m_Scene;
    Image &renderedScene;

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<Buffer> m_postProcessUbo;
    vkutil::RenderType m_renderType;
};
