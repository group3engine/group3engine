//
// Created by thomas on 28/12/24.
//

#ifndef VULKANTIME_LIGHT_HPP
#define VULKANTIME_LIGHT_HPP


#include "glsl.hpp"
#include "vulkan/vulkan.h"
#include "glm/vec4.hpp"
#include "ShadowLightManager.hpp"

// Forward declaration of ShadowLightManager
namespace GraphicsThings {
    class ShadowLightManager;
}

namespace GraphicsThings
{
    class Light
    {
    public:
        // default constructor
        Light() = default;


        Light(glm::vec4 position, glm::vec4 color) : mPosition(position), mColor(color)
        {
            enable();
        }

        Light(glm::vec4 position, glm::vec3 direction, glm::vec4 color) : mPosition(position), mColor(color), mDirection(glm::normalize(direction)), mIsDirectional(true)
        {
            enable();
        }
        virtual ~Light()
        {
            disable();

        }

        [[nodiscard]] glm::vec4 getPosition() const
        { return mPosition; }

        [[nodiscard]] glm::vec4 getColor() const
        { return mColor; }

        [[nodiscard]] glm::vec4 getDirection() const
        { return glm::vec4(mDirection, 1.0); }



        void enable();

        void disable();

        static void update_lighting_uniforms();

        static void create_sample_lights(ShadowLightManager *aShadowLightManager);

        static void destroy_lights();

        static Light *lights[MAX_LIGHTS];
        static Light *shadowLights[MAX_LIGHTS];
        static Light *directionalLights[MAX_LIGHTS];
        static Light *directionalShadowLights[MAX_LIGHTS];
        static size_t numLights;
        static size_t numShadowLights;
        static size_t numDirectionalLights;
        static size_t numDirectionalShadowLights;

        static glsl::LightingUniform lightingUniforms;
        static glsl::LightUniform lightUniforms;
    protected:
        glm::vec4 mPosition{};

        glm::vec4 mColor{};

        glm::vec3 mDirection{};

        int mLightNumber{};

        bool mEnabled = false;

        bool mIsShadow = false;

        bool mIsDirectional = false;


    };

}

#endif //VULKANTIME_LIGHT_HPP
