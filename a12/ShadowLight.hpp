//
// Created by thomas on 29/12/24.
//

#ifndef VULKANTIME_SHADOWLIGHT_HPP
#define VULKANTIME_SHADOWLIGHT_HPP

#include "Light.hpp"
#include "../labutils/allocator.hpp"
#include "../labutils/vulkan_window.hpp"
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/angle.hpp"
#include "StandardMesh.hpp"
#include "Pipeline.hpp"
#include "ShadowLightManager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>


namespace lut = labutils;

// Forward declaration of ShadowLightManager
namespace GraphicsThings {
    class ShadowLightManager;
}

namespace GraphicsThings
{





    class ShadowLight : public Light
    {


    public:
        // constructor
        ShadowLight(const glm::vec4 &aPosition, const glm::vec4 &aColor, ShadowLightManager *aShadowLightManager);
        ShadowLight(const glm::vec4 &aPosition, const glm::vec4 &aColor, const glm::vec3 &aDirection, ShadowLightManager *aShadowLightManager);


        // destructor
        ~ShadowLight() override
        {
            disableShadow();
        }


        ShadowLightManager *mShadowLightManager;

        void record_shadow_map(std::vector<StandardMesh const *> const &aMeshes);


        size_t mShadowLightNumber{};


        lut::Buffer mShadowProjUBO;

        VkDescriptorSet mShadowProjectionDescriptorSet{};

        float theta = 0;
        float phi = 0;


        void update();

        void wait_on_shadow_map();

        void begin_recording_shadow_map();

        void end_recording_shadow_map();

        void submit_shadow_map();

        lut::Pipeline
        create_shadow_pipeline(lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass,
                               VkPipelineLayout aPipelineLayout,
                               char const *vertShaderPath, char const *fragShaderPath) const;





    private:

        // create the command buffer, semaphore, and fence for recording the shadow map
        VkCommandBuffer mCommandBuffer{};
        lut::Semaphore mSemaphore;
        lut::Fence mFence;


        void initialise_mapping(lut::VulkanWindow const &, lut::Allocator const &);


        void upload_uniforms();


        glm::mat4 mShadowProjectionMatrix{};
        glm::mat4 mNdcShadowProjectionMatrix{};

        [[nodiscard]] VkDescriptorSet
        create_single_matrix_descriptor_set(lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
                                            lut::Buffer const &aBuffer) const;


        void create_shadow_map_framebuffer(lut::VulkanWindow const &aWindow C5_DBGNAME_DECL());

        lut::Framebuffer mShadowMapFramebuffer;


    public:


        void disableShadow();

        void enableShadow();

        glm::mat4 getNdcShadowProjectionMatrix();
    };


} // Graphicsthings

#endif //VULKANTIME_SHADOWLIGHT_HPP
