#version 450

layout(location = 0) in vec4 WorldPos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 WorldNormal;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec2 metallicRoughness;

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

layout (set = 1, binding = 0) uniform sampler2D uTextureColour;
layout (set = 1, binding = 1) uniform sampler2D uTextureMetallicRoughness;


void main()
{
	vec4 color = texture(uTextureMetallicRoughness, uv);
	metallicRoughness.r = color.b; // b = metallic
	metallicRoughness.g = color.g; // g = roughness
}
