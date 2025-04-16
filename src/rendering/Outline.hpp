#pragma once
#include "Volk.hpp"
#include "Image.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"

#include "Config.hpp"

class Context;

class Outline {
public:
    explicit Outline(Context &context, Scene *scene, Image& depthBuffer, Image& normalRoughnessImage);
    ~Outline();
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
private:
    Context &m_context;
    Scene *m_Scene;
    uint32_t m_width;
    uint32_t m_height;
    Image m_RenderTarget;
    Image& m_depthBuffer;
    Image& m_normalRoughness;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkDescriptorSetLayout m_playerDescriptorSetLayout;
    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> m_playerDescriptorSets;

    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;
    std::vector<Buffer> m_outlineUniform;



};

