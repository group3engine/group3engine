//
// Created by thomas on 28/12/24.
//

#include "Light.hpp"

#include "ShadowLight.hpp"

using namespace GraphicsThings;

size_t Light::numLights = 0;
size_t Light::numShadowLights = 0;
size_t Light::numDirectionalLights = 0;
size_t Light::numDirectionalShadowLights = 0;
// initialise the lights array to nullptr
Light *Light::lights[MAX_LIGHTS] = {nullptr};
Light *Light::shadowLights[MAX_LIGHTS] = {nullptr};
Light *Light::directionalLights[MAX_LIGHTS] = {nullptr};
Light *Light::directionalShadowLights[MAX_LIGHTS] = {nullptr};

glsl::LightingUniform Light::lightingUniforms = {};
glsl::LightUniform Light::lightUniforms = {};

void Light::update_lighting_uniforms() {
    // set the ambient light
    lightingUniforms.ambientLight = {0.01f, 0.01f, 0.01f};

    lightingUniforms.numLights = (int)numLights;
    for (std::size_t i = 0; i < numLights; i++) {
        lightUniforms.lights[i].position = lights[i]->getPosition();
        lightUniforms.lights[i].color = lights[i]->getColor();
    }
    lightingUniforms.numShadowLights = (int)numShadowLights;
    for (std::size_t i = 0; i < numShadowLights; i++) {
        lightUniforms.shadowLights[i].position = shadowLights[i]->getPosition();
        lightUniforms.shadowLights[i].color = shadowLights[i]->getColor();
        lightUniforms.shadowLights[i].shadowProj =
            dynamic_cast<ShadowLight *>(shadowLights[i])
                ->getNdcShadowProjectionMatrix();
    }
    lightingUniforms.numDirectionalLights = (int)numDirectionalLights;
    for (std::size_t i = 0; i < numDirectionalLights; i++) {
        lightUniforms.directionalLights[i].direction =
            dynamic_cast<Light *>(directionalLights[i])->getDirection();
        lightUniforms.directionalLights[i].color =
            dynamic_cast<Light *>(directionalLights[i])->getColor();
    }
    lightingUniforms.numDirectionalShadowLights =
        (int)numDirectionalShadowLights;
    for (std::size_t i = 0; i < numDirectionalShadowLights; i++) {
        lightUniforms.directionalShadowLights[i].direction =
            dynamic_cast<Light *>(directionalShadowLights[i])->getDirection();
        lightUniforms.directionalShadowLights[i].color =
            dynamic_cast<Light *>(directionalShadowLights[i])->getColor();
        lightUniforms.directionalShadowLights[i].shadowProj =
            dynamic_cast<ShadowLight *>(directionalShadowLights[i])
                ->getNdcShadowProjectionMatrix();
    }
}

void Light::create_sample_lights(ShadowLightManager *aShadowLightManager) {
    // Assign values to each index
    // the lights[i] = operator is not needed because the constructor adds the
    // light to the array but it is here for clarity
//        new ShadowLight(glm::vec4(0.000000, 5.645049, -1.509875, 1.0),
//                        glm::vec4(1.1, 1.1, 1.0, 1.0) * 1000.0f,
//                        aShadowLightManager);
        new Light(glm::vec4(-1.980466f, -0.816010f, -10.311163f, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-5.752258, -0.657567, -12.202829, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-7.867776, -0.903491, -15.863552, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-3.206386, -0.914726, -25.742586, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(5.705901, -0.945821, -12.332610, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(8.023162, -0.967761, -16.089260, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(2.092568, -0.885999, -10.038385, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(1.784889, -3.388978, -86.883141, 1.0),
                  glm::vec4(0.2, 0.8, 0.5, 1.0) * 10.0f);
        new Light(glm::vec4(-0.298666, -3.476735, -86.957268, 1.0),
                  glm::vec4(0.8, 0.2, 0.4, 1.0) * 10.0f);
        new Light(glm::vec4(-2.080472, -3.476735, -86.814362, 1.0),
                  glm::vec4(0.3, 0.2, 0.8, 1.0) * 10.0f);
        new Light(glm::vec4(3.137208, -0.920224, -25.853916, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-7.351207, -0.924421, -35.995827, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(2.346241, -2.983927, -46.375755, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-2.399181, -2.824856, -46.556858, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-7.181664, -3.041210, -61.131893, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(7.144320, -2.994109, -61.116470, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-1.978475, -2.962606, -66.593445, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(-2.040212, -2.986459, -68.866119, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(1.962276, -3.026429, -68.748451, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(2.113535, -3.060510, -66.729477, 1.0),
                  glm::vec4(0.8, 0.5, 0.0, 1.0) * 10.0f);
        new Light(glm::vec4(0.094273, -3.790991, -49.635178, 1.0),
                  glm::vec4(1.96, 0.2, 0.1, 1.0) * 10.0f);  // fire pit
        new Light(glm::vec4(-0.062715, -3.869835, -75.897560, 1.0),
                  glm::vec4(1.96, 0.2, 0.1, 1.0) * 10.0f);  // fire pit

//        new ShadowLight(glm::vec4(0.285695, 7.887019, -35.968216, 1.0),
//                        glm::vec4(0.0, 0.5, 0.8, 1.0) * 1000.0f,
//                        aShadowLightManager);
//        aShadowLightManager->shadowLights[1]->phi = glm::radians(45.0f);

    // add a directional light pointing 45 degrees down
    new ShadowLight(glm::vec4(0.000000, 15.324938, 31.535208, 1.0),
                    glm::vec4(0.0, 0.5, 0.8, 1.0) * 3.0f,
                    glm::vec3(0.0, 1.0, 1.0), aShadowLightManager);
}

void Light::enable() {
    if (!mEnabled) {
        if (mIsShadow) {
            if (mIsDirectional) {
                if (directionalShadowLights[numDirectionalShadowLights] !=
                        nullptr &&
                    directionalShadowLights[numDirectionalShadowLights] !=
                        this) {
                    delete directionalShadowLights[numDirectionalShadowLights];
                }
                directionalShadowLights[numDirectionalShadowLights] = this;
                mLightNumber = (int)numDirectionalShadowLights;
                numDirectionalShadowLights++;
                mEnabled = true;
            } else {
                if (shadowLights[numShadowLights] != nullptr &&
                    shadowLights[numShadowLights] != this) {
                    delete shadowLights[numShadowLights];
                }
                shadowLights[numShadowLights] = this;
                mLightNumber = (int)numShadowLights;
                numShadowLights++;
                mEnabled = true;
            }
        } else {
            if (mIsDirectional) {
                if (directionalLights[numDirectionalLights] != nullptr &&
                    directionalLights[numDirectionalLights] != this) {
                    delete directionalLights[numDirectionalLights];
                }
                directionalLights[numDirectionalLights] = this;
                mLightNumber = (int)numDirectionalLights;
                numDirectionalLights++;
                mEnabled = true;

            } else {
                if (lights[numLights] != nullptr && lights[numLights] != this) {
                    delete lights[numLights];
                }
                lights[numLights] = this;
                mLightNumber = (int)numLights;
                numLights++;
                mEnabled = true;
            }
        }
    }
}

void Light::disable() {
    if (mEnabled) {
        if (!mIsDirectional) {
            if (mIsShadow) {
                numShadowLights--;
                shadowLights[mLightNumber] = shadowLights[numShadowLights];
                if (shadowLights[mLightNumber] != nullptr)
                    shadowLights[mLightNumber]->mLightNumber = mLightNumber;
                mEnabled = false;
            } else {
                numLights--;
                lights[mLightNumber] = lights[numLights];
                if (lights[mLightNumber] != nullptr)
                    lights[mLightNumber]->mLightNumber = mLightNumber;
                mEnabled = false;
            }
        } else {
            if (mIsShadow) {
                numDirectionalShadowLights--;
                directionalLights[mLightNumber] =
                    directionalLights[numDirectionalLights];
                if (directionalLights[mLightNumber] != nullptr)
                    directionalLights[mLightNumber]->mLightNumber =
                        mLightNumber;
                mEnabled = false;
            } else {
                numDirectionalLights--;
                directionalLights[mLightNumber] =
                    directionalLights[numDirectionalLights];
                if (directionalLights[mLightNumber] != nullptr)
                    directionalLights[mLightNumber]->mLightNumber =
                        mLightNumber;
                mEnabled = false;
            }
        }
    }
}
void Light::destroy_lights() {
    for (size_t i = 0; i < numLights; i++) {
        delete lights[i];
    }
    for (size_t i = 0; i < numShadowLights; i++) {
        delete shadowLights[i];
    }
    for (size_t i = 0; i < numDirectionalLights; i++) {
        delete directionalLights[i];
    }
    for (size_t i = 0; i < numDirectionalShadowLights; i++) {
        delete directionalShadowLights[i];
    }
    numLights = 0;
    numShadowLights = 0;
}
