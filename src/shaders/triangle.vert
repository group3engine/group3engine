#version 450

#include "uniforms.glsl"

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

// TODO: Add model matrix

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec3 oWorldPos;
layout(location = 1) out vec4 oColor;

void main()
{
    // Get world position
    vec4 pos = vec4(vPos, 1.0f);
    // vec4 world_pos = ubo.model * pos;
    vec4 world_pos = pos;

    // Transform the position from world space to homogeneous projection space
    vec4 proj_pos = ubo.view * world_pos;
    proj_pos = ubo.projection * proj_pos;
    gl_Position = proj_pos;

    // output world position of the vertex
    oWorldPos = world_pos.xyz;

    oColor = vColor;
}
