//#pragma once
//
// ===============================================================================================
// This should no longer be needed. ForwardPass writes to the target we need for post processing
// ===============================================================================================
//#include "Volk.hpp"
//#include <memory>
//#include <unordered_map>
//#include "Camera.hpp"
//
//#include "Config.hpp"
//
///*
//* This is a thin g-buffer required for post-processing such as SSAO and SSR
//*/
//
//class Context;
//class Scene;
//class Buffer;
//
//class GBuffer {
//  public:
//
//    GBuffer(Context &context, Scene *scene);
//    ~GBuffer();
//    void Execute(VkCommandBuffer cmd);
//    void Update();
//
//    void Resize();
//
//    Image& GetMetallicRoughnessTarget() { return m_RenderTarget; }
//
//  private:
//    void CreatePipeline();
//    void CreateRenderPass();
//    void CreateFramebuffer();
//    void BuildDescriptorSetLayouts();
//    void BuildDescriptors();
//
//    uint32_t m_width;
//    uint32_t m_height;
//
//    Image m_RenderTarget;
//    Image m_DepthTarget;
//    VkRenderPass m_renderPass;
//    VkFramebuffer m_framebuffer;
//
//    Context &context;
//    Scene *scene;
//    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> mPlayerDescriptorSets;
//    VkDescriptorSetLayout mPlayerDescriptorSetLayout;
//
//    VkPipeline m_Pipeline;
//    VkPipelineLayout m_PipelineLayout;
//
//    VkPipeline m_AlphaMaskingPipeline;
//    VkPipelineLayout m_AlphaMaskingPipelineLayout;
//};
