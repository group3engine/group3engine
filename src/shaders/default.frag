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
// material numbers
layout (set = 1, binding = 3) uniform UNumbers
{
	vec4 baseColour;
	float metallness;
	float roughness;
	float alphaCutoff;

} uNumbers;

#define PI 3.14159265359

uint cascadeIndex = 0;

vec3 Fresnel(vec3 halfVector, vec3 viewDir, vec3 baseColor, float metallic)
{
    vec3 F0 = vec3(0.04);
    F0 = (1 - metallic) * F0 + (metallic * baseColor);
    float HdotV = max(dot(halfVector, viewDir), 0.0);
    vec3 schlick_approx = F0 + (1 - F0) * pow(clamp(1 - HdotV, 0.0, 1.0), 5);
    return schlick_approx;
}

struct SHCoefficients {
    vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
};

//const SHCoefficients grace = SHCoefficients(
//    vec3( 0.7953949,  0.4405923,  0.5459412 ),
//    vec3( 0.3981450,  0.3526911,  0.6097158 ),
//    vec3(-0.3424573, -0.1838151, -0.2715583 ),
//    vec3(-0.2944621, -0.0560606,  0.0095193 ),
//    vec3(-0.1123051, -0.0513088, -0.1232869 ),
//    vec3(-0.2645007, -0.2257996, -0.4785847 ),
//    vec3(-0.1569444, -0.0954703, -0.1485053 ),
//    vec3( 0.5646247,  0.2161586,  0.1402643 ),
//    vec3( 0.2137442, -0.0547578, -0.3061700 )
//);

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

vec3 evaluateSH(vec3 normal) {
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
vec3 CookTorranceBRDF(vec3 normal, vec3 halfVector, vec3 viewDir, vec3 lightDir, float metallic, float roughness, vec3 baseColor, vec3 LightColour)
{
    vec3 F = Fresnel(halfVector, viewDir, baseColor, metallic);
    float D = GGXNormalDistributionFunction(normal, halfVector, roughness);
	float G = GGXGeometrySmith(normal, lightDir, viewDir, roughness);

    vec3 L_Diffuse = (baseColor / PI) * (vec3(1.0) - F) * (1.0 - metallic) * evaluateSH(normal);

    float NdotV = max(dot(normal, viewDir), 0.001);
	float NdotL = max(dot(normal, lightDir), 0.001);

	vec3 numerator = D * G * F;
	float denominator = (4 * NdotV * NdotL) + 0.001;

	vec3 specular = numerator / denominator;
    specular = specular * LightColour.xyz * NdotL;

    // band it to 0, 0.25, 0.5, 0.75, 1.0
    float specularLength = length(specular);
    vec3 specularColour = normalize(specular);
    float specularRed = specularColour.r / 2;
    float specularGreen = specularColour.g / 2;
    float specularBlue = specularColour.b / 2;
    float newRed = 0.0;
    float newGreen = 0.0;
    float newBlue = 0.0;
    if(specularRed < 0.05)
    {
        newRed = 0.0;
    }
    else if(specularRed < 0.25)
    {
        newRed = 0.1;
    }
    else if(specularRed < 0.6)
    {
        newRed = 0.3;
    }
    else if(specularRed < 0.8)
    {
        newRed = 0.5;
    }
    else if (specularRed > 0.8)
    {
        newRed = 1.0;
    }

    if(specularGreen < 0.05)
    {
        newGreen = 0.0;
    }
    else if(specularGreen < 0.25)
    {
        newGreen = 0.1;
    }
    else if(specularGreen < 0.6)
    {
        newGreen = 0.3;
    }
    else if(specularGreen < 0.8)
    {
        newGreen = 0.5;
    }
    else if (specularGreen > 0.8)
    {
        newGreen = 1.0;
    }

    if(specularBlue < 0.05)
    {
        newBlue = 0.0;
    }
    else if(specularBlue < 0.25)
    {
        newBlue = 0.1;
    }
    else if(specularBlue < 0.6)
    {
        newBlue = 0.3;
    }
    else if(specularBlue < 0.8)
    {
        newBlue = 0.5;
    }
    else if (specularBlue > 0.8)
    {
        newBlue = 1.0;
    }

    vec3 specularFinal = vec3(newRed, newGreen, newBlue);

    vec3 outLight = (L_Diffuse) * LightColour.xyz * NdotL;
    outLight += specularFinal;

    return vec3(outLight);
}


const vec2 PCFFilter4x4[16] = vec2[](
vec2(-1.5, 1.5), vec2(-0.5, 1.5), vec2(0.5, 1.5), vec2(1.5, 1.5),
vec2(-1.5, 0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5), vec2(1.5, 0.5),
vec2(-1.5, -0.5), vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(1.5, -0.5),
vec2(-1.5, -1.5), vec2(-0.5, -1.5), vec2(0.5, -1.5), vec2(1.5, -1.5)
);


//float PCF(vec4 shadowMapPosition)
//{
//	vec2 offset = vec2(shadowMapPosition.w / 1024.0f);
//	float shadow = 0.0;
//	for (int i = 0; i < 16; i++)
//	{
//        vec4 pcfShadowMapPosition = shadowMapPosition + vec4(PCFFilter4x4[i] * offset, 0.0, 0.0);
//        shadow += textureProj(shadowMap, shadowMapPosition);
//	}
//
//	return shadow / 16.0;
//}

// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
//float Shadows(vec3 WorldPos)
//{
//	// Use direct lighting only. Point light shadows are handleded differently (cube depth)
//	vec4 fragPositionInLightSpace = lightData.lights[0].LightSpaceMatrix * vec4(WorldPos, 1.0);
//	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
//	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;
//	fragPositionInLightSpace.z = fragPositionInLightSpace.z - 0.005;
//	float shadow = PCF(fragPositionInLightSpace);
//
//	return shadow;
//}

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
// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
float myPCF(vec3 WorldPos)
{
	// Use direct lighting only. Point light shadows are handleded differently (cube depth)
//	vec4 fragPositionInLightSpace = lightData.lights[0].LightSpaceMatrix * vec4(WorldPos, 1.0);
//	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
//	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;

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
    float shadowValue = sum / float(samples);
    // band the shadow map by threshold 0.5
    float shadow = step(0.5, shadowValue);

    return shadow;
}

void main()
{
    #ifdef ALPHA
    if (texture(uTextureColour, uv).a < uNumbers.alphaCutoff)
        discard;
    #endif
    vec3 color = texture(uTextureColour, uv).rgb * uNumbers.baseColour.rgb;
    vec3 emissive = vec3(0.0);

    // == Metal and Roughness ==
    float roughness = texture(uTextureMetallicRoughness, uv).g * uNumbers.roughness;
    float metallic = texture(uTextureMetallicRoughness, uv).b * uNumbers.metallness;

    // What if there is no normal map?
    vec3 pixelNormal = normalize(TBNFrame * (texture(uTextureNormal, uv).xyz * 2.f - 1.f));

    vec3 outLight = vec3(0.0);

    {
        int i = 0;

        vec3 lightDir = normalize(lightData.lights[i].LightPosition.xyz);
        vec3 viewDir = normalize(ubo.cameraPosition.xyz - WorldPos.xyz);
        vec3 halfVector = normalize(viewDir + lightDir);

        vec3 LightColour = lightData.lights[i].LightColour.rgb;

        float shadowTerm = 1.0 - myPCF(WorldPos.xyz);
        vec3 brdf = CookTorranceBRDF(pixelNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour);
        outLight += brdf * LightColour.xyz * shadowTerm;
    }

//    for (int i = 1; i < NUM_LIGHTS; i++)
//    {
//        vec3 lightDir = normalize(lightData.lights[i].LightPosition.xyz - WorldPos.xyz);
//        vec3 viewDir = normalize(ubo.cameraPosition.xyz - WorldPos.xyz);
//        vec3 halfVector = normalize(viewDir + lightDir);
//
//        float dist = length(lightData.lights[i].LightPosition.xyz - WorldPos.xyz);
//        float att = 1.0 / (dist * dist);
//        vec3 LightColour = lightData.lights[i].LightColour.xyz * att;
//
//        float shadowTerm = 1.0;
//        vec3 brdf = CookTorranceBRDF(pixelNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour);
//        outLight += brdf * LightColour.xyz * shadowTerm;
//    }

    vec3 ambient = vec3(0.02) * color;

    outLight = ambient + outLight + emissive;
    float dist = length(ubo.cameraPosition - WorldPos);
    float heightFalloff = exp(-max(0.0, WorldPos.y) * 0.05);
    float fogDensity = 0.01 * heightFalloff;
    float fogFactor = exp(-pow(dist * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 sunColor = evaluateSH(pixelNormal.rgb); // use Spherical harmonics to get diffuse IBL in this dir
    vec3 sunDirection = normalize(lightData.lights[0].LightPosition.xyz);
    float scattering = max(0.0, dot(normalize(ubo.cameraPosition.xyz - WorldPos.xyz), sunDirection));
    vec3 fogColor = mix(vec3(0.3, 0.3, 0.4), sunColor, scattering * 0.3);

    //fragColor.rgb = mix(fogColor, outLight, fogFactor);
    //fragColor.rgb = evaluateSH(((pixelNormal.xyz)));

    fragColor.rgb = outLight;
    //fragColor = vec4(vec3(ambient + outLight + emissive), 1.0);
    NormalMetallic = vec4(pixelNormal.xyz * 0.5 + 0.5, roughness);




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
