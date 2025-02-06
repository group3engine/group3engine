#include "pbr_default.glsl"
// push constant for the model matrix
layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
} pushConstants;


layout (location = 0) in vec2 v2fTexCoord;
layout (location = 1) in vec3 v2fPosition;
layout (location = 2) in vec3 v2fNormal;

layout (set = 0, binding = 0) uniform UScene
{
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
} uScene;

// colour texture
layout (set = 1, binding = 0) uniform sampler2D uTextureColour;
// roughness texture
layout (set = 1, binding = 1) uniform sampler2D uTextureRoughness;
// metalness texture
layout (set = 1, binding = 2) uniform sampler2D uTextureMetalness;
// alpha mask texture (not used yet)
layout (set = 1, binding = 3) uniform sampler2D uTextureAlphaMask;
// normal texture (not used yet)
layout (set = 1, binding = 4) uniform sampler2D uTextureNormal;
// emissive texture (not used yet)
layout (set = 1, binding = 5) uniform sampler2D uTextureEmissive;

// lighting information
// ambient light for the scene (one value for all objects) and the number of lights
layout (set = 2, binding = 0) uniform ULighting
{
    vec3 ambientLight;
    int numLights;
    int numShadowLights;
    int numDirectionalLights;
    int numDirectionalShadowLights;
} uLighting;
struct PointLightShadowed
{
    vec4 position;
    vec4 colour;
    mat4 shadowMapProjectionMatrix;
};
struct PointLight
{
    vec4 position;
    vec4 colour;
};
struct DirectionalLight
{
    vec4 direction;
    vec4 colour;
};
struct DirectionalLightShadowed
{
    vec4 direction;
    vec4 colour;
    mat4 shadowMapProjectionMatrix;
};
// the set of lights in the scene
layout (set = 2, binding = 1) uniform ULights
{
    PointLight lights[MAX_LIGHTS];
    PointLightShadowed shadowLights[MAX_SHADOWS];
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    DirectionalLightShadowed directionalShadowLights[MAX_DIRECTIONAL_SHADOWS];

} uLights;

// the set of shadow maps
layout (set = 3, binding = 0) uniform sampler2DShadow uShadowMap[MAX_LIGHTS];

float isInShadow(vec4 shadowMapPosition, int index)
{

    //TODO: this is a hack because setting the border colour to black doesn't work
    if (shadowMapPosition.x < 0 || shadowMapPosition.x > shadowMapPosition.w || shadowMapPosition.y < 0 || shadowMapPosition.y > shadowMapPosition.w)
    {
        return 0.0;
    }
    else
    {
        // sample the shadow map with textureproj
        return textureProj(uShadowMap[index], shadowMapPosition);


    }
}


const vec2 PCFFilter4x4[16] = vec2[](
vec2(-1.5, 1.5), vec2(-0.5, 1.5), vec2(0.5, 1.5), vec2(1.5, 1.5),
vec2(-1.5, 0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5), vec2(1.5, 0.5),
vec2(-1.5, -0.5), vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(1.5, -0.5),
vec2(-1.5, -1.5), vec2(-0.5, -1.5), vec2(0.5, -1.5), vec2(1.5, -1.5)
);


float PCF(vec4 shadowMapPosition, int index)
{
    vec2 offset = vec2(shadowMapPosition.w / 1024.0f);
    float shadow = 0.0;
    for (int i = 0; i < 16; i++)
    {
        shadow += isInShadow(shadowMapPosition + vec4(PCFFilter4x4[i] * offset, 0.0, 0.0), index);
    }

    return shadow / 16.0;
}




vec4 ComputeLighting()
{
    #ifdef ALPHA
    // get the alpha mask
    float alphaMask = texture(uTextureAlphaMask, v2fTexCoord).a;
    // if the alpha mask is less than 0.1, discard the fragment
    if (alphaMask < 0.1f)
    {
        discard;
    }
    #endif

    vec3 normal = normalize(v2fNormal);


    // get the per-fragment data from the textures
    // roughness
    float roughness = texture(uTextureRoughness, v2fTexCoord).r;
    roughness = roughness * roughness;
    // metalness
    float metalness = texture(uTextureMetalness, v2fTexCoord).r;
    // base colour
    vec3 baseColour = texture(uTextureColour, v2fTexCoord).rgb;
    // emissive
    vec3 emissive = texture(uTextureEmissive, v2fTexCoord).rgb;

    // view direction
    vec3 viewDirection = normalize(uScene.cameraPosition.xyz - v2fPosition);


    vec3 light = vec3(0);
    // for each light, calculate the lighting
    for (int i = 0; i < uLighting.numLights; i++)
    {
        ComputeAllLighting(v2fPosition, viewDirection, baseColour, roughness, metalness, normal, uLights.lights[i].position, uLights.lights[i].colour, light);
    }
    // for each shadow light, calculate the lighting
    for (int i = 0; i < uLighting.numShadowLights; i++)
    {
        // calculate if its in shadow
        vec4 shadowMapPosition = uLights.shadowLights[i].shadowMapProjectionMatrix * vec4(v2fPosition, 1.0);
        shadowMapPosition /= shadowMapPosition.w;
        float inShadow = PCF(shadowMapPosition, i);
        ComputeAllLightingWithShadows(v2fPosition, viewDirection, baseColour, roughness, metalness, normal, uLights.shadowLights[i].position, uLights.shadowLights[i].colour, light, inShadow);
    }
    // for each directional light, calculate the lighting
    for (int i = 0; i < uLighting.numDirectionalLights; i++)
    {
        ComputeAllLightingDirectional(v2fPosition, viewDirection, baseColour, roughness, metalness, normal, uLights.directionalLights[i].direction.xyz, uLights.directionalLights[i].colour, light);

    }
    // for each directional shadow light, calculate the lighting
    for (int i = 0; i < uLighting.numDirectionalShadowLights; i++)
    {
        // calculate if its in shadow
        vec4 shadowMapPosition = uLights.directionalShadowLights[i].shadowMapProjectionMatrix * vec4(v2fPosition, 1.0);
        shadowMapPosition /= shadowMapPosition.w;
        float inShadow = PCF(shadowMapPosition, 0);
        ComputeAllLightingDirectionalWithShadows(v2fPosition, viewDirection, baseColour, roughness, metalness, normal, uLights.directionalShadowLights[i].direction.xyz, uLights.directionalShadowLights[i].colour, light, inShadow);
    }

    // add the ambient light
    light += uLighting.ambientLight * baseColour;


    light += emissive * 75.f;

    // output the final colour
    return vec4(light, 1.f);

}