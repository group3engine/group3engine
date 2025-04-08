#pragma once
#include "Volk.hpp"
#include <memory>
#include <unordered_map>
#include "Camera.hpp"

#include "Config.hpp"

class Context;
class Scene;
class Buffer;


class ShadowMap {
  public:
    ShadowMap(Context &context, Scene *scene);
    ~ShadowMap();
    void Execute(VkCommandBuffer cmd);
    void Update();

    Image &GetRenderTarget() { return m_ShadowMap; }
    static constexpr uint8_t NUM_SHADOW_CASCADES = 4;

    struct Cascade
    {
        VkFramebuffer framebuffer;
        VkImageView imgView;
        float splitDepth;
        glm::mat4 viewProjMatrix;
        void Destroy(VkDevice device)
        {
            vkDestroyImageView(device, imgView, nullptr);
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    const std::vector<Cascade>& GetCascades() { return m_Cascades; }
    const std::vector<Buffer>& GetCascadeUniformBuffer() const { return m_CascadeUniformBuffer; }

  private:
    void CreatePipeline();
    void CreateRenderPass();
    void CreateFramebuffer();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();

    VkRenderPass m_renderPass;
    VkFramebuffer m_framebuffer;

    Context &context;
    Image m_ShadowMap;
    uint32_t m_width;
    uint32_t m_height;

    Scene *scene;
    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> mPlayerDescriptorSets;
    VkDescriptorSetLayout mPlayerDescriptorSetLayout;
    VkDescriptorSetLayout skinDescriptorSetLayout;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;

    VkPipeline m_SkinnedPipeline;
    VkPipelineLayout m_SkinnedPipelineLayout;

    // CSM
    //std::vector<VkImageView> m_CascadeImageViews;
    //std::vector<VkFramebuffer> m_CascadeFramebuffer;
    std::vector<Cascade> m_Cascades;

    const float cascadeSplitLambda = 0.95f;
    std::vector<Buffer> m_CascadeUniformBuffer;
    vkutil::CascadeMatrices m_cascadeMatricesData;
};
