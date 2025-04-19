#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 0) uniform CameraUBO
{
	mat4 view;
	mat4 projection;
	mat4 inverseProjection;
	mat4 inverseView;
    vec4 cameraPosition;
    vec2 viewportSize;
	float fov;
	float nearPlane;
	float farPlane;
} ubo;

layout(set = 1, binding = 1) uniform FogSettings
{
    float MaxDistance;
    float Density;
    float StepSize;
    int MaxSteps;
}fog;

layout (set = 1, binding = 0) uniform sampler2D depthBuffer;
layout (set = 1, binding = 2) uniform sampler2D renderedScene;
layout (set = 1, binding = 3) uniform samplerCube skybox;

#define NUM_SHADOW_CASCADES 4

layout(set = 1, binding = 4) uniform CascadeMatrices
{
	mat4 cascadeViewProjection[NUM_SHADOW_CASCADES];
	vec4 cascadeSplits;
}csmMatrices;

layout(set = 1, binding = 5) uniform sampler2DArrayShadow shadowMap;

uint cascadeIndex = 0;

vec4 DepthToPosition(vec2 uv)
{
	float depth = texture(depthBuffer, uv).x;
	vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewSpace = ubo.inverseProjection * clipSpace;
	viewSpace.xyz /= viewSpace.w;

	return vec4(viewSpace.xyz, 1.0);
}

float isShadow(vec3 WorldPos)
{
    vec4 fragPositionInLightSpace = csmMatrices.cascadeViewProjection[cascadeIndex] * vec4(WorldPos, 1.0);
	fragPositionInLightSpace.xyz /= fragPositionInLightSpace.w;
	fragPositionInLightSpace.xy = fragPositionInLightSpace.xy * 0.5 + 0.5;

    float currentDepth = fragPositionInLightSpace.z;
    vec4 sampleCoord = vec4(fragPositionInLightSpace.xy, float(cascadeIndex), fragPositionInLightSpace.z);
    float shadow = texture(shadowMap, sampleCoord);

    return currentDepth > shadow + 0.001 ? 1.0 : 0.0;
}

vec3 random_pcg3d(uvec3 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v ^= v >> 16u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  return vec3(v) * (1.0/float(0xffffffffu));
}

vec4 VolFog()
{
    vec4 WorldPos = ubo.inverseView * vec4(DepthToPosition(uv).xyz, 1.0);
    vec3 viewDir =  WorldPos.xyz - ubo.cameraPosition.xyz;
    float dist = length(viewDir);
    vec3 RayDir = normalize(viewDir);

    float maxDistance = min(dist, (fog.MaxDistance));
    float distTravelled = random_pcg3d(uvec3(gl_FragCoord.xy, 0)).x * fog.StepSize;
    float transmittance = 1.0;

    float density = fog.Density;
    vec3 finalColour = vec3(0);
    vec3 LightColour = vec3(0.6, 0.75, 1.0); //  This is a sky like colour
    while(distTravelled < maxDistance)
    {
        vec3 currentPos = ubo.cameraPosition.xyz + RayDir * distTravelled;
        float visbility = isShadow(currentPos);
        finalColour += LightColour * 1.0 * density * fog.StepSize * visbility; // Removing visiblitiy for now since there is a issue with the camera causing flickering. Not sure why.
        transmittance *= exp(-density * fog.StepSize);
        distTravelled += fog.StepSize;
    }

    vec4 sceneColour = texture(renderedScene, uv);
    transmittance = clamp(transmittance, 0.0, 1.0);
    // return mix(sceneColour.rgb, finalColour, 1.0 - transmittance);

    return vec4(finalColour, 1.0 - transmittance);
}

void main()
{
    vec4 viewPos = vec4(DepthToPosition(uv).xyz, 1.0);
    for(uint i = 0; i < NUM_SHADOW_CASCADES - 1; ++i)
    {
        cascadeIndex = viewPos.z < csmMatrices.cascadeSplits[i] ? cascadeIndex = i + 1: cascadeIndex;
    }

   fragColour = vec4(VolFog());
//   switch(cascadeIndex) {
//		case 0 :
//			fragColour.rgb *= vec3(1.0f, 0.25f, 0.25f);
//			break;
//		case 1 :
//			fragColour.rgb *= vec3(0.25f, 1.0f, 0.25f);
//			break;
//		case 2 :
//			fragColour.rgb *= vec3(0.25f, 0.25f, 1.0f);
//			break;
//		case 3 :
//			fragColour.rgb *= vec3(1.0f, 1.0f, 0.25f);
//			break;
//	}
}
