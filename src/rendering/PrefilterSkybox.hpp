#pragma once
#include "Buffer.hpp"
#include "Camera.hpp"
#include "Image.hpp"
#include "Volk.hpp"

#include "Config.hpp"

class Context;
class PrefilterSkybox
{
  public:
    explicit PrefilterSkybox(Context &context, Image &skybox);
    ~PrefilterSkybox();
    void Execute(VkCommandBuffer cmd);

    struct MipLevels
    {
        VkImageView imageView;
        void Destroy(VkDevice device) {
            vkDestroyImageView(device, imageView, nullptr);
        }
    };


    Image &GetPrefilteredSkybox() { return m_PrefilteredSkybox; }
    Image &GetBRDFLut() { return m_BRDFLut; }

    void TransitionResources(VkCommandBuffer cmd);

  private:

    void CreatePrefilterPipeline();
    void CreateBRDFLUTPipeline();
    void BuildPrefilterDescriptorSets();
    void BuildBRDFLUTDescriptorSets();

    struct PushConstants {
        float roughness;
        uint32_t mipLevel;
        float baseResolution;
    };

    Image m_BRDFLut;
    std::vector<MipLevels> mipLevelImages;
    Image m_PrefilteredSkybox;

    uint32_t m_width;
    uint32_t m_height;

    uint32_t mipLevels;
    Context &context;
    Image &skybox;

    // Prefilter cubemap resources
    VkPipeline m_PrefilterSkyboxPipeline;
    VkPipelineLayout m_PrefilterSkyboxPipelineLayout;
    VkDescriptorSetLayout m_PrefilterSkyboxDescriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    // BRDF LUT resources
    VkPipeline m_brdfLUTPipeline;
    VkPipelineLayout m_brdfLUTPipelineLayout;
    VkDescriptorSetLayout m_brdfLUTDescriptorSetLayout;
    std::vector<VkDescriptorSet> brdfLUTDescriptorSets;

};