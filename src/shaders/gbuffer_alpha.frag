#version 450

layout(location = 0) in vec4 WorldPos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 WorldNormal;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec4 albedo;

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

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
}pc;

layout (set = 1, binding = 0) uniform sampler2D uTextureColour;

void main()
{
	// need to check alpha in this and discard
	vec4 color = texture(uTextureColour, uv);
	albedo = color;
}
