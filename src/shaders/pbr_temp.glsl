#include "pbr_default.glsl"



layout (location = 0) in vec2 v2fTexCoord;
layout (location = 1) in vec3 v2fPosition;
layout (location = 2) in vec3 v2fNormal;

layout (location = 0) out vec4 oColour;
layout(location = 1) out vec4 brightColours;

layout(set = 0, binding = 0) uniform SceneUniform
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
    vec2 viewportSize;
    float fov;
    float nearPlane;
    float farPlane;
} ubo;

struct Light
{
    int Type;
    vec4 LightPosition;
    vec4 LightColour;
    mat4 LightSpaceMatrix;
};

const int NUM_LIGHTS = 26;

layout(set = 0, binding = 1) uniform LightBuffer {
    Light lights[NUM_LIGHTS];
} lightData;



// colour texture
layout (set = 1, binding = 0) uniform sampler2D uTextureColour;
// roughness texture
layout (set = 1, binding = 1) uniform sampler2D uTextureMetallicRoughness;
// material numbers
layout (set = 1, binding = 2) uniform UNumbers
{
    vec4 baseColour;
    float metallness;
    float roughness;
    float alphaCutoff;

} uNumbers;




vec4 ComputeLighting()
{
    #ifdef ALPHA
    // get the alpha mask
    float alphaMask = texture(uTextureColour, v2fTexCoord).a;
    // if the alpha mask is less than the alpha cutoff, discard the fragment
    if (alphaMask < uNumbers.alphaCutoff)
    {
        discard;
    }
    #endif

    vec3 normal = normalize(v2fNormal);


    // get the per-fragment data from the textures
    // roughness
    float roughness = texture(uTextureMetallicRoughness, v2fTexCoord).g * uNumbers.roughness;
    roughness = roughness * roughness;
    // metalness
    float metalness = texture(uTextureMetallicRoughness, v2fTexCoord).b * uNumbers.metallness;
    // base colour
    vec3 baseColour = texture(uTextureColour, v2fTexCoord).rgb * uNumbers.baseColour.rgb;
    // emissive
    vec3 emissive = vec3(0.0, 0.0, 0.0);

    // view direction
    vec3 viewDirection = normalize(ubo.cameraPosition.xyz - v2fPosition);


    vec3 light = vec3(0);
    // for each light, calculate the lighting
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        ComputeAllLighting(v2fPosition, viewDirection, baseColour, roughness, metalness, normal, lightData.lights[i].LightPosition, lightData.lights[i].LightColour, light);
    }




    light += emissive * 75.f;

    // output the final colour
    return vec4(light, 1.f);

}