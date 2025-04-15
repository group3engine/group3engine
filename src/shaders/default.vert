#version 450

#include "uniforms.glsl"
#include "Common.glsl"

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
}pc;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;

layout(location = 0) out vec4 WorldPos;
layout(location = 1) out vec2 uv;
layout(location = 2) out mat3 TBNFrame;

void main()
{
	TBNFrame = tbn(normal, tangent, pc.ModelMatrix);

	uv = tex;
	WorldPos = pc.ModelMatrix * vec4(pos, 1.0);
	gl_Position = ubo.projection * ubo.view * pc.ModelMatrix * vec4(pos, 1.0);
}
