#version 450

#include "uniforms.glsl"

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
}pc;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;

layout(location = 0) out vec3 texCoords;

void main()
{
	texCoords = pos;
	mat4 rotView = mat4(mat3(ubo.view));
	vec4 upos = ubo.projection * rotView * pc.ModelMatrix * vec4(pos, 1.0);
	gl_Position = upos.xyww;
}
