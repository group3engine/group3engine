#ifdef VERSION
#version 450
#define VERSION
#endif

layout(location = 0) in vec4 WorldPos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 WorldNormal;

layout(location = 0) out vec4 fragColor;
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

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
}pc;

layout(set = 0, binding = 2) uniform sampler2DShadow shadowMap;
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

#define PI 3.14159265359

#define FRESNEL(halfVector, viewDir, baseColor, metallic, schlick_approx) { \
    vec3 F0 = vec3(0.04); \
    F0 = (1 - (metallic)) * F0 + ((metallic) * (baseColor)); \
    float HdotV = max(dot((halfVector), (viewDir)), 0.0); \
    schlick_approx = F0 + (1 - F0) * pow(clamp(1 - HdotV, 0.0, 1.0), 5); \
}

// ======================================================================
// GGX
// https://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
// ======================================================================

#define GGX_NORMAL_DISTRIBUTION_FUNCTION(N, H, roughness, D) { \
    float a = (roughness) * (roughness); \
    float a2 = a * a; \
    float NdotH = max(dot((N), (H)), 0.001); \
    float NdotH2 = NdotH * NdotH; \
    float numerator = a2; \
    float denominator = ((NdotH2) * (a2 - 1.0) + 1.0); \
    denominator = PI * denominator * denominator; \
    D = numerator / max(denominator, 0.001); \
}

#define GGX_GEOMETRY_SCHLICK(NdotV, roughness, ggx) { \
    float a = (roughness) * (roughness); \
    float a2 = a * a; \
    float NdotV2 = (NdotV) * (NdotV); \
    float numerator = 2.0 * (NdotV); \
    float denominator = (NdotV) + sqrt(a2 + (1.0 - a2) * NdotV2); \
    ggx = numerator / max(denominator, 0.001); \
}

#define GGX_GEOMETRY_SMITH(normal, lightDir, viewDir, roughness, G) { \
    float NdotV = max(dot((normal), (viewDir)), 0.001); \
    float NdotL = max(dot((normal), (lightDir)), 0.001); \
    float ggx1;\
    float ggx2;\
    GGX_GEOMETRY_SCHLICK(NdotV, (roughness), ggx1); \
    GGX_GEOMETRY_SCHLICK(NdotL, (roughness), ggx2); \
    G = ggx1 * ggx2; \
}

// Compute BRDF
#define COOK_TORRENCE_BRDF(normal, halfVector, viewDir, lightDir, metallic, roughness, baseColor, LightColour, brdf) {\
    vec3 F;\
    FRESNEL(halfVector, viewDir, baseColor, metallic, F);\
    float D;\
    GGX_NORMAL_DISTRIBUTION_FUNCTION(normal, halfVector, roughness, D);\
	float G;\
    GGX_GEOMETRY_SMITH(normal, lightDir, viewDir, roughness, G);\
    vec3 L_Diffuse = (baseColor / PI) * (vec3(1.0) - F) * (1.0 - metallic);\
    float NdotV = max(dot(normal, viewDir), 0.001);\
	float NdotL = max(dot(normal, lightDir), 0.001);\
	vec3 numerator = D * G * F;\
	float denominator = (4 * NdotV * NdotL) + 0.001;\
	vec3 specular = numerator / denominator;\
    vec3 outLight = (L_Diffuse + specular) * NdotL;\
    brdf = vec3(outLight);\
}

const vec2 PCFFilter4x4[16] = vec2[](
vec2(-1.5, 1.5), vec2(-0.5, 1.5), vec2(0.5, 1.5), vec2(1.5, 1.5),
vec2(-1.5, 0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5), vec2(1.5, 0.5),
vec2(-1.5, -0.5), vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(1.5, -0.5),
vec2(-1.5, -1.5), vec2(-0.5, -1.5), vec2(0.5, -1.5), vec2(1.5, -1.5)
);


float PCF(vec4 shadowMapPosition)
{
	vec2 offset = vec2(shadowMapPosition.w / 1024.0f);
	float shadow = 0.0;
	for (int i = 0; i < 16; i++)
	{
        vec4 pcfShadowMapPosition = shadowMapPosition + vec4(PCFFilter4x4[i] * offset, 0.0, 0.0);
        shadow += textureProj(shadowMap, shadowMapPosition);
	}

	return shadow / 16.0;
}

// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
float Shadows(vec3 WorldPos)
{
	// Use direct lighting only. Point light shadows are handleded differently (cube depth)
	vec4 fragPositionInLightSpace = lightData.lights[0].LightSpaceMatrix * vec4(WorldPos, 1.0);
	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;
	fragPositionInLightSpace.z = fragPositionInLightSpace.z - 0.005;
	float shadow = PCF(fragPositionInLightSpace);

	return shadow;
}

float Shadow(vec3 WorldPos)
{
	vec4 fragPositionInLightSpace = lightData.lights[0].LightSpaceMatrix * vec4(WorldPos, 1.0);
	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;
	fragPositionInLightSpace.z -= 0.005;

	float shadow = textureProj(shadowMap, fragPositionInLightSpace);
	return shadow;
}

// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
float myPCF(vec3 WorldPos)
{
	// Use direct lighting only. Point light shadows are handleded differently (cube depth)
	vec4 fragPositionInLightSpace = lightData.lights[0].LightSpaceMatrix * vec4(WorldPos, 1.0);
	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;

	vec2 texSize = 1.0 / textureSize(shadowMap, 0);
	int range = 2; // 4x4
	int samples = 0;
	float sum = 0.0;
	for(int x = -range; x < range; x++)
	{
		for(int y = -range; y < range; y++)
		{
			vec2 offset = vec2(x,y) * texSize;
			vec4 sampleCoord = vec4(fragPositionInLightSpace.xy + offset, fragPositionInLightSpace.z, fragPositionInLightSpace.w);
			sum += textureProj(shadowMap, sampleCoord);
			samples++;
		}
	}

	return sum / float(samples);
}

void main()
{
    vec3 color = texture(uTextureColour, uv).rgb * uNumbers.baseColour.rgb;
    vec3 emissive = vec3(0.0);

    // == Metal and Roughness ==
    float roughness = texture(uTextureMetallicRoughness, uv).g * uNumbers.roughness;
    float metallic = texture(uTextureMetallicRoughness, uv).b * uNumbers.metallness;

    vec3 outLight = vec3(0.0);

    for(int i = 0; i < NUM_LIGHTS; i++)
    {
        vec3 lightDir = normalize(lightData.lights[i].LightPosition.xyz - WorldPos.xyz);
        vec3 viewDir = normalize(ubo.cameraPosition.xyz - WorldPos.xyz);
        vec3 halfVector = normalize(viewDir + lightDir);

        // is it a spot light?
        vec3 LightColour = vec3(0.0);
        bool isDirectional = lightData.lights[i].Type == 1 ? false : true;

        if (!isDirectional)
        {
            float dist = length(lightData.lights[i].LightPosition.xyz - WorldPos.xyz);
            float att = 1.0 / (dist * dist);
            LightColour = lightData.lights[i].LightColour.xyz * att;
        }
        else
        {
            lightDir = normalize(lightData.lights[i].LightPosition.xyz);
            LightColour = lightData.lights[i].LightColour.rgb;
            halfVector = normalize(viewDir + lightDir);
        }

        float shadowTerm = 1.0;
        if (isDirectional)
        {
            shadowTerm = 1.0 - myPCF(WorldPos.xyz);
        }

        vec3 brdf;
        COOK_TORRENCE_BRDF(WorldNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour, brdf);
        outLight += brdf * LightColour.xyz * shadowTerm;
    }

    vec3 ambient = vec3(0.02) * color;
    fragColor = vec4(vec3(ambient + outLight), 1.0);

    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
    brightColours = vec4(fragColor.rgb, 1.0);
    else
    brightColours = vec4(0.0, 0.0, 0.0, 1.0);
}
