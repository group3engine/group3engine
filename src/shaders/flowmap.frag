#ifdef VERSION
#version 450
#define VERSION
#endif

#include "uniforms.glsl"
#include "Common.glsl"

layout(location = 0) in vec4 WorldPos;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat3 TBNFrame;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 brightColours;
layout(location = 2) out vec4 NormalMetallic;

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

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

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
    int cascadeIndex;
    float t;
}pc;

#define NUM_SHADOW_CASCADES 4

layout(set = 0, binding = 3) uniform CascadeMatrices
{
	mat4 cascadeViewProjection[NUM_SHADOW_CASCADES];
	vec4 cascadeSplits;
}csmMatrices;


layout (set = 0, binding = 4) readonly buffer SHBuffer
{
    vec3 shCoefficients[9];
}sh;

layout(set = 0, binding = 2) uniform sampler2DArrayShadow shadowMap;

// colour texture
layout (set = 1, binding = 0) uniform sampler2D uTextureColour;
// roughness texture
layout (set = 1, binding = 1) uniform sampler2D uTextureMetallicRoughness;
// normal map
layout (set = 1, binding = 2) uniform sampler2D uTextureNormal;
// emissive texture
layout (set = 1, binding = 3) uniform sampler2D uTextureEmissive;
// material numbers
layout (set = 1, binding = 4) uniform UNumbers
{
	vec4 baseColour;
	float metallness;
	float roughness;
	float alphaCutoff;
    vec4 emissiveFactor;
} uNumbers;

layout (set = 0, binding = 5) uniform samplerCube prefilteredSkybox;
layout (set = 0, binding = 6) uniform sampler2D BRDFLUT;
layout (set = 0, binding = 7) uniform samplerCube irradianceMap;

layout (set = 0, binding = 8) uniform RendererDebug
{
    int debugMode;
}debugRenderer;

layout(set = 2, binding = 0) uniform sampler2D textureFlowMap;
layout(set = 2, binding = 1) uniform sampler2D textureFlowMapNoise;

#define PI 3.14159265359

uint cascadeIndex = 0;

// colors for each mip level
const vec4 colors[10] = {
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(0.0, 0.0, 1.0, 1.0),
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(0.0, 1.0, 1.0, 1.0),
    vec4(1.0, 0.0, 1.0, 1.0),
    vec4(1.0, 0.5, 0.0, 1.0),
    vec4(0.5, 0.0, 1.0, 1.0),
    vec4(0.5, 0.5, 0.5, 1.0),
    vec4(1.0, 1.0, 1.0, 1.0)
};

vec3 DebugMipColour()
{
    vec2 lodInfo = textureQueryLod(uTextureColour, uv);
    int numMips = textureQueryLevels(uTextureColour);

    float lod = max(lodInfo.y, 0.0); // clamping this because of texture has no mips as is the case with our 1x1 default it'll return garbage .y value
    int index = int(floor(lod));
    float blendFactor = fract(lod);

    int clampedIndex = clamp(index + 1, 0, numMips - 1);
    vec4 baseColor = colors[index];
    vec4 nextColor = colors[clampedIndex];

    vec4 blendedColor = mix(baseColor, nextColor, blendFactor);

    return blendedColor.rgb;
}

vec3 FresnelSchlickWithRoughness(float cosTheta, vec3 F0, float roughness)
{
    vec3 fresnel = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    return fresnel;
}

vec3 Fresnel(vec3 halfVector, vec3 viewDir, vec3 baseColor, float metallic)
{
    vec3 F0 = vec3(0.04);
    F0 = (1 - metallic) * F0 + (metallic * baseColor);
    float HdotV = max(dot(halfVector, viewDir), 0.0);
    vec3 schlick_approx = F0 + (1 - F0) * pow(clamp(1 - HdotV, 0.0, 1.0), 5);
    return schlick_approx;
}

vec3 FresnelSchlick(float HdotV, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

struct SHCoefficients {
    vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

vec3 EvaluateSHForDiffuseIBL(vec3 normal) {
    vec3 result = vec3(0.0);
    result += sh.shCoefficients[0] * SH00();
    result += sh.shCoefficients[1] * SH1m1(normal);
    result += sh.shCoefficients[2] * SH10(normal);
    result += sh.shCoefficients[3] * SH11(normal);
    result += sh.shCoefficients[4] * SH2m2(normal);
    result += sh.shCoefficients[5] * SH2m1(normal);
    result += sh.shCoefficients[6] * SH20(normal);
    result += sh.shCoefficients[7] * SH21(normal);
    result += sh.shCoefficients[8] * SH22(normal);
    return result;
}

SHCoefficients grace = SHCoefficients(
    sh.shCoefficients[0],
    sh.shCoefficients[1],
    sh.shCoefficients[2],
    sh.shCoefficients[3],
    sh.shCoefficients[4],
    sh.shCoefficients[5],
    sh.shCoefficients[6],
    sh.shCoefficients[7],
    sh.shCoefficients[8]
);


vec3 calcIrradiance(vec3 nor) {
    const SHCoefficients c = grace;
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    return (
        c1 * c.l22 * (nor.x * nor.x - nor.y * nor.y) +
        c3 * c.l20 * nor.z * nor.z +
        c4 * c.l00 -
        c5 * c.l20 +
        2.0 * c1 * c.l2m2 * nor.x * nor.y +
        2.0 * c1 * c.l21  * nor.x * nor.z +
        2.0 * c1 * c.l2m1 * nor.y * nor.z +
        2.0 * c2 * c.l11  * nor.x +
        2.0 * c2 * c.l1m1 * nor.y +
        2.0 * c2 * c.l10  * nor.z
    );
}

// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
float PCF(vec3 WorldPos)
{
	vec3 texSize = 1.0 / textureSize(shadowMap, 0);
    // compute the cascade index
    vec4 viewPos = ubo.view * vec4(WorldPos, 1.0);
    for(uint i = 0; i < NUM_SHADOW_CASCADES - 1; ++i)
    {
        cascadeIndex = viewPos.z < csmMatrices.cascadeSplits[i] ? cascadeIndex = i + 1: cascadeIndex;
    }

    vec4 fragPositionInLightSpace = csmMatrices.cascadeViewProjection[cascadeIndex] * vec4(WorldPos, 1.0);
	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;

	int range = 2; // 4x4
	int samples = 0;
	float sum = 0.0;
	for(int x = -range; x < range; x++)
	{
		for(int y = -range; y < range; y++)
		{
			vec2 offset = vec2(x,y) * texSize.xy;
			vec4 sampleCoord = vec4(fragPositionInLightSpace.xy + offset, fragPositionInLightSpace.z, fragPositionInLightSpace.w);
			sum += texture(shadowMap, vec4(sampleCoord.xy, float(cascadeIndex), sampleCoord.z)); // I don't think textureProj works with sampler2DArrayShadow
            samples++;
		}
	}

	return sum / float(samples);
}


// ======================================================================
// GGX
// https://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// ======================================================================

float GGXNormalDistributionFunction(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.001);
	float NdotH2 = NdotH * NdotH;

	float numerator = a2;
	float denominator = ((NdotH2) * (a2 - 1.0) + 1.0);
	denominator = PI * denominator * denominator;

	return numerator / max(denominator, 0.001);
}

float GGXGeometrySchlick(float NdotV, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotV2 = NdotV * NdotV;

	float numerator   = 2.0 * NdotV;
	float denominator = NdotV + sqrt(a2 + (1.0 - a2) * NdotV2);

	return numerator / max(denominator, 0.001);
}

float GGXGeometrySmith(vec3 normal, vec3 lightDir, vec3 viewDir, float roughness)
{
	float NdotV = max(dot(normal, viewDir), 0.001);
	float NdotL = max(dot(normal, lightDir), 0.001);

	float ggx1 = GGXGeometrySchlick(NdotV, roughness);
	float ggx2 = GGXGeometrySchlick(NdotL, roughness);

	return ggx1 * ggx2;
}

// Compute BRDF
vec3 CookTorranceBRDF(vec3 normal, vec3 halfVector, vec3 viewDir, vec3 lightDir, float metallic, float roughness, vec3 baseColor, vec3 LightColour, vec3 WorldPos)
{
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallic);

    vec3 F = FresnelSchlick(max(dot(halfVector, viewDir), 0.0), F0);
    float D = GGXNormalDistributionFunction(normal, halfVector, roughness);
	float G = GGXGeometrySmith(normal, lightDir, viewDir, roughness);

    vec3 kd = (1.0 - F) * (1.0 - metallic);
    vec3 L_Diffuse = kd * (baseColor / PI);

    float NdotV = max(dot(normal, viewDir), 0.001);
	float NdotL = max(dot(normal, lightDir), 0.001);

	vec3 numerator = D * G * F;
	float denominator = (4 * NdotV * NdotL) + 0.001;
	vec3 specular = numerator / denominator;

    vec3 directLight = (kd * baseColor / PI + specular) * LightColour.xyz * NdotL;

    vec3 FR = FresnelSchlickWithRoughness(NdotV, F0, roughness);
    vec3 R = reflect(-viewDir, normal);
    const float max_specular_mip_levels = 7.0;

    vec3 prefilteredColour = textureLod(prefilteredSkybox, R, roughness * max_specular_mip_levels).rgb;
    vec2 envBRDF = texture(BRDFLUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL = prefilteredColour * (FR * envBRDF.x + envBRDF.y);

    vec3 iblKD = (1.0 - FR) * (1.0 - metallic);
    vec3 irradiance = texture(irradianceMap, normal).rgb; // EvaluateSHForDiffuseIBL(normal);
    vec3 diffuseIBL = irradiance * baseColor * iblKD;
    vec3 indirectLight = diffuseIBL + specularIBL;

    directLight = directLight;
    return vec3(directLight + indirectLight);
}

void main()
{
    // ----
    vec4 flow_map_x_channel = vec4(1.0, 0.0, 0.0, 0.0);
    vec4 flow_map_y_channel = vec4(0.0, 1.0, 0.0, 0.0);
    vec4 noise_texture_channel = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 channel_flow_direction = vec2(1.0, -1.0);
    float blend_cycle = 1.0;
    float cycle_speed = 0.1;
    float flow_speed =  1.25;

    // UV flow  calculation
    /****************************************************************************************************/
    float half_cycle = blend_cycle * 0.5;

    // Use noise texture for offset to reduce pulsing effect
    float flow_noise_size = 1.0;
    float flow_noise_influence = 1.0;
    float offset = texture(textureFlowMapNoise, uv * flow_noise_size).r * flow_noise_influence;
    // float offset = 0.0;

    float phase1 = blend_cycle - abs(mod((offset + pc.t * cycle_speed), (2 * blend_cycle)) - blend_cycle);
    float phase2 = mod(offset + pc.t * cycle_speed + half_cycle, blend_cycle);

    vec4 flow_tex = texture(textureFlowMap, uv);
    vec2 flow;
    flow.x = dot(flow_tex, flow_map_x_channel) * 2.0 - 1.0;
    flow.y = dot(flow_tex, flow_map_y_channel) * 2.0 - 1.0;
    flow *= normalize(channel_flow_direction);

    // Make flow incluence on the normalmap strenght adjustable (optional)
    float flow_normal_influence = 0.5;
    float normal_influence = mix(1.0, dot(abs(flow), vec2(1.0, 1.0)) * 0.5, flow_normal_influence);

    // Blend factor to mix the two layers
    float blend_factor = abs(half_cycle - phase1)/half_cycle;

    // Offset by halfCycle to improve the animation for color (for normalmap not absolutely necessary)
    phase1 -= half_cycle;
    phase2 -= half_cycle;

    vec2 uv_scale = vec2(1.0, 1.0);
    // Multiply with scale to make flow speed independent from the uv scaling
    flow *= flow_speed * uv_scale;

    vec2 layer1 = flow * phase1 + uv;
    vec2 layer2 = flow * phase2 + uv;

    // Mix animated uv layers for color
    vec3 color = mix(texture(uTextureColour, layer1), texture(uTextureColour, layer1), blend_factor).rgb * uNumbers.baseColour.rgb;

    // Mix emissive uv layers
    vec3 emissive = uNumbers.emissiveFactor.rgb * mix(texture(uTextureEmissive, layer1), texture(uTextureEmissive, layer1), blend_factor).rgb;

    // == Metal and Roughness ==
    float roughness = mix(texture(uTextureMetallicRoughness, layer1).g, texture(uTextureMetallicRoughness, layer1).g, blend_factor)  * uNumbers.roughness;
    float metallic = mix(texture(uTextureMetallicRoughness, layer1).b, texture(uTextureMetallicRoughness, layer1).b, blend_factor) * uNumbers.metallness;
    vec3 texNormal = mix(texture(uTextureNormal, layer1).xyz, texture(uTextureNormal, layer1).xyz, blend_factor);
    vec3 pixelNormal = normalize(TBNFrame * (texNormal * 2.f - 1.f));

    vec3 outLight = vec3(0.0);

    vec3 lightDir = normalize(lightData.lights[0].LightPosition.xyz);
    vec3 viewDir = normalize(ubo.cameraPosition.xyz - WorldPos.xyz);
    vec3 halfVector = normalize(viewDir + lightDir);

    vec3 LightColour = lightData.lights[0].LightColour.rgb;
    vec3 brdf = CookTorranceBRDF(pixelNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour, WorldPos.xyz);
    outLight += brdf;

    // add the emissive light
    float pulseFactor = ((sin(0.7 * pc.t) + 1.0) / 2.0) / 2.0 + 0.5;
    // float pulseFactor = 1.0;
    outLight += emissive * pulseFactor;

    // This is no longer needed since we now have IBL which is the "indirect"
    // Keeping this here for reference
    // vec3 ambient = vec3(0.02) * color;

    fragColor = vec4(outLight, 1.0);
    // fragColor = vec4(texture(textureFlowMap, uv).rgb, 1.0);
    NormalMetallic = vec4(pixelNormal.xyz * 0.5 + 0.5, roughness);

    switch(debugRenderer.debugMode) {
        case 1:
            fragColor = vec4(pixelNormal.xyz * 0.5 + 0.5, 1.0);
            break;
        case 2:
            fragColor = vec4(WorldPos.xyz, 1.0);
            break;
        case 3:
            fragColor = vec4(color.rgb, 1.0);
            break;
        case 4:
            fragColor = vec4(vec3(roughness), 1.0);
            break;
        case 5:
            fragColor = vec4(vec3(metallic), 1.0);
            break;
        case 6:
            fragColor = vec4(vec3(1.0 - PCF(WorldPos.xyz)), 1.0);
            break;
        case 7:
            fragColor = vec4(DebugMipColour(), 1.0);
            break;
        case 8:
            switch(cascadeIndex) {
    		case 0 :
    			fragColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
    			break;
    		case 1 :
    			fragColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
    			break;
    		case 2 :
    			fragColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
    			break;
    		case 3 :
    			fragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
    			break;
    	    }
    }

//    switch(cascadeIndex) {
//		case 0 :
//			fragColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
//			break;
//		case 1 :
//			fragColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
//			break;
//		case 2 :
//			fragColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
//			break;
//		case 3 :
//			fragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
//			break;
//	}

    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    float threshold = step(1.0, brightness); // check if brightness is greater than 1.0
    brightColours = vec4(fragColor.rgb * threshold, 1.0);
}
