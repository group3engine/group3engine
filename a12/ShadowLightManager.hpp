//
// Created by thomas on 21/01/25.
//

#ifndef VULKANTIME_SHADOWLIGHTMANAGER_HPP
#define VULKANTIME_SHADOWLIGHTMANAGER_HPP

#include "../labutils/dbgname.h"
#include "../labutils/vkimage.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/dbgname.h"
#include "Pipeline.hpp"
#include "StandardMesh.hpp"
#include "glsl.hpp"

#define ASSETDIR_ "assets/a12/"
#   		define SHADERDIR_ ASSETDIR_ "shaders/"
constexpr char const *KShadowMapVertexShaderPath = SHADERDIR_ "shadowmap.vert.spv";
constexpr char const *KShadowMapFragmentShaderPath = SHADERDIR_ "shadowmap.frag.spv";
#undef SHADERDIR_
#undef ASSETDIR_

namespace lut = labutils;

// forward declaration of ShadowLight
namespace GraphicsThings
{
    class ShadowLight;
}

namespace GraphicsThings
{

    class ShadowLightManager
    {
    public:

        ShadowLightManager(lut::VulkanWindow *aWindow, lut::Allocator *aAllocator);
        ~ShadowLightManager();

        ShadowLight *shadowLights[MAX_LIGHTS] = {nullptr};
        size_t numShadowLights = 0;
        VkExtent2D shadowMapExtent = {1024, 1024};
        lut::Sampler shadowSampler;
        lut::DescriptorPool shadowDescriptorPool;
        lut::CommandPool commandPool;

        lut::RenderPass shadowRenderPass;

        Pipeline<lut::Pipeline (*)(lut::VulkanWindow const &, VkRenderPass, VkPipelineLayout, char const *,
                                   char const *, VkExtent2D), char const *, char const *, VkExtent2D> *shadowMapPipeline = nullptr;

        lut::PipelineLayout shadowMapPipelineLayout;

        lut::DescriptorSetLayout allShadowMapDescriptorSetLayout;
        lut::DescriptorSetLayout matrixDescriptorSetLayout;

        VkDescriptorSet allShadowMapDescriptorSet{};


        // array of images and image views for the shadow maps
        lut::ImageView shadowLightViews[MAX_LIGHTS];
        lut::Image shadowLightImages[MAX_LIGHTS];



        void create_all_shadow_descriptor_layout(const lut::VulkanWindow &aWindow);

        void create_all_shadow_descriptor_set(const lut::VulkanWindow &aWindow);

        void create_shadow_render_pass(lut::VulkanWindow const &aWindow C5_DBGNAME_DECL());

        void create_shadow_map_pipeline_layout();

        static lut::Sampler create_shadow_sampler(lut::VulkanContext const &aContext C5_DBGNAME_DECL());

        void create_single_matrix_descriptor_layout(lut::VulkanWindow const &aWindow);

        std::tuple<lut::Image, lut::ImageView>
        create_depth_buffer(const lut::VulkanWindow &, const lut::Allocator &) const;


        lut::VulkanWindow *window;

        lut::Allocator *allocator;

    };

    lut::Pipeline
    create_shadow_pipeline(lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass,
                           VkPipelineLayout aPipelineLayout,
                           char const *vertShaderPath, char const *fragShaderPath, VkExtent2D shadowMapExtent);


}



#endif //VULKANTIME_SHADOWLIGHTMANAGER_HPP
