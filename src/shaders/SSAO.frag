#version 450

#include "uniforms.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

layout(set = 1, binding = 1) uniform SSAOSettings
{
	int NumDirections;
	int NumSteps;
	float Radius;
	float StepSize;
	float intensity;
}ssao;


layout (set = 1, binding = 0) uniform sampler2D depthBuffer;
layout (set = 1, binding = 2) uniform sampler2D normalRoughness;
layout (set = 1, binding = 3) uniform sampler2D NoiseTexture;

#define PI 3.14159265359


void main()
{
	float ao = 1.0;
	float occlusion = 0.0;

    fragColour = vec4(vec3(ao), 1.0);
}
