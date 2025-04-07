#version 450

struct Light
{
	int Type;
	vec4 LightPosition;
	vec4 LightColour;
	mat4 LightSpaceMatrix;
};

const int NUM_LIGHTS = 26;

layout(set = 0, binding = 0) uniform LightBuffer {
	Light lights[NUM_LIGHTS];
} lightData;

#define NUM_SHADOW_CASCADES 4

layout(set = 0, binding = 1) uniform CascadeMatrices
{
	mat4 cascadeViewProjection[NUM_SHADOW_CASCADES];
	vec4 splitCascades;
}csmMatrices;

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
	int cascadeIndex;
}pc;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;

void main()
{
//lightData.lights[0].LightSpaceMatrix
	gl_Position = csmMatrices.cascadeViewProjection[pc.cascadeIndex] * pc.ModelMatrix * vec4(pos, 1.0);
}
