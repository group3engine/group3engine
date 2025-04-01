//
// Created by thomas on 01/04/25.
//

#ifndef GROUP3ENGINE_LIGHTMANAGER_HPP
#define GROUP3ENGINE_LIGHTMANAGER_HPP


#include <cstdlib>
#include <bitset>
#include "Light.hpp"
#include "Buffer.hpp"
/// The LightManager class is responsible for managing the lights in the scene.
/// The LightManager class is a singleton
class LightManager {
public:
    /// returns a pointer to the singleton instance of the LightManager
    static LightManager& getInstance() {
        static LightManager instance;
        return instance;
    }

public:
    /// returns a pointer to the first available light, and the index of the light, used when returning the light to the pool
    /// returns <nullptr, -1> if no light is available
    std::tuple<Light*, int> GetDirectionalLight();
    /// returns a pointer to the first available light, and the index of the light, used when returning the light to the pool
    /// returns <nullptr, -1> if no light is available
    std::tuple<Light*, int> GetPointLight();
    /// Sets the next available directional light to the passed in light
    /// returns a reference to the light, and the index of the light
    /// returns <nullptr, -1> if no light is available
    std::tuple<Light*, int> SetDirectionalLight(Light* light);
    /// Sets the next available point light to the passed in light
    /// returns a reference to the light, and the index of the light
    /// returns <nullptr, -1> if no light is available
    std::tuple<Light*, int> SetPointLight(Light* light);
    /// returns a directional light to the pool
    void ReturnDirectionalLight(int index);
    /// returns a point light to the pool
    void ReturnPointLight(int index);

    void Update();

    void UploadLights(VkCommandBuffer cmdBuff);

    void Destroy();

    void StartUp(Context &aContext);

    std::vector<Buffer> &GetLightsUBO() { return m_LightUBO; }
    std::vector<Light*> GetLights();


private:
    LightManager() = default;
    ~LightManager() = default;
private:
    // the directional lights
    Light mDirectionalLights[NUM_DIRECTIONAL_LIGHTS];
    // the mask of enabled lights
    std::bitset<NUM_DIRECTIONAL_LIGHTS> mDirectionalLightMask = 0;
    // the point lights
    Light mPointLights[NUM_POINT_LIGHTS];
    // the mask of enabled lights
    std::bitset<NUM_POINT_LIGHTS> mPointLightMask = 0;
    // the light buffer data
    LightBuffer m_LightBuffer = {};
    // the ring buffer (size num frames in flight) of light data
    std::vector<Buffer> m_LightUBO;



};


#endif //GROUP3ENGINE_LIGHTMANAGER_HPP
