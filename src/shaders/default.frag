#version 450

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

// Fresnel (shlick approx)
vec3 Fresnel(vec3 halfVector, vec3 viewDir, vec3 baseColor, float metallic)
{
    vec3 F0 = vec3(0.04);
    F0 = (1 - metallic) * F0 + (metallic * baseColor);
    float HdotV = max(dot(halfVector, viewDir), 0.0);
    vec3 schlick_approx = F0 + (1 - F0) * pow(clamp(1 - HdotV, 0.0, 1.0), 5);
    return schlick_approx;
}

// Normal distribution function
float BeckmannNormalDistribution(vec3 normal, vec3 halfVector, float roughness)
{
    float a = roughness * roughness;
	float a2 = a * a; // alpha is roughness squared
	float NdotH = max(dot(normal, halfVector), 0.001); // preventing divide by zero
	float NdotHSquared = NdotH * NdotH;
	float numerator = exp((NdotHSquared - 1.0) / (a2 * NdotHSquared));
	float denominator = PI * a2 * (NdotHSquared * NdotHSquared); // pi * a2 * (n * h)^4

	float D = numerator / denominator;
	return D;
}

// Geometry term
float GeometryTerm(vec3 normal, vec3 halfVector, vec3 lightDir, vec3 viewDir)
{
	float NdotH = max(dot(normal, halfVector), 0.0);
	float NdotV = max(dot(normal, viewDir), 0.0);
	float VdotH = max(dot(viewDir, halfVector), 0.0);
	float NdotL = max(dot(normal, lightDir), 0.0);

	float term1 = 2 * (NdotH * NdotV) / VdotH;
	float term2 = 2 * (NdotH * NdotL) / VdotH;

	float G = min(1, min(term1, term2));

	return G;
}

// Compute BRDF
vec3 CookTorranceBRDF(vec3 normal, vec3 halfVector, vec3 viewDir, vec3 lightDir, float metallic, float roughness, vec3 baseColor, vec3 LightColour)
{
    vec3 F = Fresnel(halfVector, viewDir, baseColor, metallic);
    float D = BeckmannNormalDistribution(normal, halfVector, roughness);
	float G = GeometryTerm(normal, halfVector, lightDir, viewDir);

    vec3 ambient = vec3(0.02);
    vec3 L_Diffuse = (baseColor.xyz / PI) * (vec3(1,1,1) - F) * (1.0 - metallic);

    float NdotV = max(dot(normal, viewDir), 0.0);
	float NdotL = max(dot(normal, lightDir), 0.0);

	vec3 numerator = D * G * F;
	float denominator = (4 * NdotV * NdotL) + 0.001;

	vec3 specular = numerator / denominator;

    vec3 outLight = (L_Diffuse + specular) * LightColour.xyz * NdotL;

    return vec3(outLight);
}

float isInShadow(vec4 shadowMapPosition)
{

		// sample the shadow map with textureproj
		return textureProj(shadowMap, shadowMapPosition);

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
		shadow += isInShadow(shadowMapPosition + vec4(PCFFilter4x4[i] * offset, 0.0, 0.0));
	}

	return shadow / 16.0;
}

// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
float Shadows(vec3 WorldPos)
{
    vec3 lightDir = normalize(lightData.lights[0].LightPosition.xyz);
	//lightDir = -lightDir;

	// Use direct lighting only. Point light shadows are handleded differently (cube depth)
	vec4 fragPositionInLightSpace = lightData.lights[0].LightSpaceMatrix * vec4(WorldPos, 1.0);
	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;
	fragPositionInLightSpace.z = fragPositionInLightSpace.z - 0.01;
	float shadow = PCF(fragPositionInLightSpace);
	return 0.0;
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

void main()
{
	vec3 color = texture(uTextureColour, uv).rgb * uNumbers.baseColour.rgb;
	vec3 emissive = vec3(0.0);

    // == Metal and Roughness ==
	float roughness = texture(uTextureMetallicRoughness, uv).g * uNumbers.roughness;
	float metallic = texture(uTextureMetallicRoughness, uv).b * uNumbers.metallness;

    vec3 outLight = vec3(0.0);

	for(int i = 0; i < 1; i++)
	{
		vec3 lightDir = normalize(lightData.lights[i].LightPosition.xyz);
		vec3 viewDir = normalize(ubo.cameraPosition.xyz - WorldPos.xyz);
		vec3 halfVector = normalize(viewDir + lightDir);

		float diff = max(dot(WorldNormal, lightDir), 0.0);
		outLight += diff * color * lightData.lights[i].LightColour.rgb;

		// is it a spot light?
		vec3 LightColour = vec3(0.0);
		bool isDirectional = lightData.lights[i].Type == 1 ? false : true;

		if(!isDirectional)
		{
			float dist = length(lightData.lights[i].LightPosition.xyz - WorldPos.xyz);
			float att = 1.0 / (dist * dist);
			LightColour = lightData.lights[i].LightColour.xyz * att;
		}
		else {
			lightDir = normalize(lightData.lights[i].LightPosition.xyz);
			LightColour = lightData.lights[i].LightColour.rgb;
		}

		if(isDirectional) {
			float shadowTerm = 1.0 - Shadows(WorldPos.xyz);
			outLight += CookTorranceBRDF(WorldNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour);

		}
		else {
			//outLight += CookTorranceBRDF(WorldNormal, halfVector, viewDir, lightDir, metallic, roughness, color, LightColour);
		}
	}

    float shadowTerm = Shadow(WorldPos.xyz);

	vec3 ambient = vec3(0.02) * color;

	fragColor = vec4(vec3(ambient + outLight), 1.0);

	float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > 1.0)
		brightColours = vec4(fragColor.rgb, 1.0);
	else
		brightColours = vec4(0.0, 0.0, 0.0, 1.0);
}
