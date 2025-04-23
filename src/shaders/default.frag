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


layout (set = 0, binding = 7) uniform RendererDebug
{
    int debugMode;
}debugRenderer;

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

vec3 Fresnel(vec3 halfVector, vec3 viewDir, vec3 baseColor, float metallic, float roughness)
{
    vec3 F0 = vec3(0.04);
    F0 = (1 - metallic) * F0 + (metallic * baseColor);
    float HdotV = max(dot(halfVector, viewDir), 0.0);
    vec3 schlick_approx = F0 + (1 - F0) * pow(clamp(1 - HdotV, 0.0, 1.0), 5);
    return schlick_approx;
    //return FresnelSchlickWithRoughness(HdotV, F0, roughness);
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
    vec3 F = Fresnel(halfVector, viewDir, baseColor, metallic, roughness);
    float D = GGXNormalDistributionFunction(normal, halfVector, roughness);
	float G = GGXGeometrySmith(normal, lightDir, viewDir, roughness);

    vec3 kd = (1.0 - F) * (1.0 - metallic);
    vec3 L_Diffuse = kd * (baseColor / PI);

    float NdotV = max(dot(normal, viewDir), 0.0);
	float NdotL = max(dot(normal, lightDir), 0.0);

	vec3 numerator = D * G * F;
	float denominator = (4 * NdotV * NdotL) + 0.001;
	vec3 specular = numerator / denominator;

    vec3 directLight = (kd * baseColor / PI + specular) * LightColour.xyz * NdotL;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallic);
    vec3 FR = FresnelSchlickWithRoughness(NdotV, F0, roughness);
    vec3 R = reflect(-viewDir, normal);
    const float max_specular_mip_levels = 11.0;

    vec3 prefilteredColour = textureLod(prefilteredSkybox, R, roughness * max_specular_mip_levels).rgb;
    vec2 envBRDF = texture(BRDFLUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL = prefilteredColour * (FR * envBRDF.x + envBRDF.y);

    vec3 diffuseIBL = kd * (EvaluateSHForDiffuseIBL(normal) * baseColor);
    vec3 indirectLight = diffuseIBL + specularIBL;

    float shadowTerm = 1.0 - PCF(WorldPos.xyz);
    directLight = directLight * shadowTerm;
    return vec3(directLight + indirectLight);
}

float Shadow(vec3 WorldPos)
{
    vec4 fragPositionInLightSpace = csmMatrices.cascadeViewProjection[cascadeIndex] * vec4(WorldPos, 1.0);
	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;
	fragPositionInLightSpace.z -= 0.005;

	float shadow = texture(shadowMap, fragPositionInLightSpace);
	return shadow;
}
//
void main()
{
    #ifdef ALPHA
    if (texture(uTextureColour, uv).a < uNumbers.alphaCutoff)
        discard;
    #endif
    vec3 color = texture(uTextureColour, uv).rgb * uNumbers.baseColour.rgb;
    vec3 emissive = uNumbers.emissiveFactor.rgb;

    // == Metal and Roughness ==
    float roughness = texture(uTextureMetallicRoughness, uv).g * uNumbers.roughness;
    float metallic = texture(uTextureMetallicRoughness, uv).b * uNumbers.metallness;
    vec3 pixelNormal = normalize(TBNFrame * (texture(uTextureNormal, uv).xyz * 2.f - 1.f));

    vec3 outLight = vec3(0.0);

    {
        int i = 0;

        vec3 lightDir = normalize(lightData.lights[i].LightPosition.xyz);
        vec3 viewDir = normalize(ubo.cameraPosition.xyz - WorldPos.xyz);
        vec3 halfVector = normalize(viewDir + lightDir);

        vec3 LightColour = lightData.lights[i].LightColour.rgb;
        vec3 brdf = CookTorranceBRDF(pixelNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour, WorldPos.xyz);
        outLight += brdf;
    }


    for (int i = 1; i < NUM_LIGHTS; i++)
    {
        vec3 lightDir = normalize(lightData.lights[i].LightPosition.xyz - WorldPos.xyz);
        vec3 viewDir = normalize(ubo.cameraPosition.xyz - WorldPos.xyz);
        vec3 halfVector = normalize(viewDir + lightDir);

        float dist = length(lightData.lights[i].LightPosition.xyz - WorldPos.xyz);
        float att = 1.0 / (dist * dist);
        vec3 LightColour = lightData.lights[i].LightColour.xyz * att;

        float shadowTerm = 1.0;
        vec3 brdf = CookTorranceBRDF(pixelNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour, WorldPos.xyz);
        outLight += brdf * LightColour.xyz * shadowTerm;
    }
    // add the emissive light
    outLight += emissive;

    // This is no longer needed since we now have IBL which is the "indirect"
    // Keeping this here for reference
    // vec3 ambient = vec3(0.02) * color;

    fragColor = vec4(outLight, 1.0);
    NormalMetallic = vec4(pixelNormal.xyz * 0.5 + 0.5, roughness);


    switch(debugRenderer.debugMode) {
        case 1:
            fragColor = vec4(pixelNormal.xyz, 1.0);
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
    float threshold = step(1.0, brightness); // check if brightness is less than 1.0
    brightColours = vec4(fragColor.rgb * threshold, 1.0);
}
