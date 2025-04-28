#pragma once
#include "Buffer.hpp"
#include "Camera.hpp"
#include "Image.hpp"
#include "Volk.hpp"

#include "Config.hpp"

class Context;
class IrradianceMap {
  public:
    explicit IrradianceMap(Context &context, Image &skybox);
    ~IrradianceMap();
    void Execute(VkCommandBuffer cmd);

    Image &GetIrradianceMap() { return m_IrradianceMap; }
    void Transition(VkCommandBuffer cmd);

  private:

   struct Layer
   {
        VkImageView imageView;
        void Destroy(VkDevice device) {
            vkDestroyImageView(device, imageView, nullptr);
        }
   };

    void CreatePipeline();
    void BuildDescriptorSets();

    struct PushConstants {
        float roughness;
        uint32_t mipLevel;
    };

    Image m_IrradianceMap;
    std::vector<Layer> m_layers;

    uint32_t m_width;
    uint32_t m_height;

    Context &context;
    Image &skybox;
    VkRenderPass m_RenderPass;


    // Prefilter cubemap resources
    VkPipeline m_IrradianceMapPipeline;
    VkPipelineLayout m_IrradianceMapPipelineLayout;
    VkDescriptorSetLayout m_IrradianceMapDescriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
};