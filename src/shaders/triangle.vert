#version 450

layout(set = 0, binding = 0) uniform SceneUniform {
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
    vec2 viewportSize;
    float fov;
    float nearPlane;
    float farPlane;
} ubo;

// TODO: Add model matrix

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTex;
layout(location = 2) in vec3 vNorm;

layout(location = 0) out vec3 oNormal;
layout(location = 1) out vec3 oWorldPos;
layout(location = 2) out vec2 oTex;
layout(location = 4) out vec4 oColor;

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

    // output normal
    vec4 norm = vec4(vNorm, 0.0f);
    // oNormal = normalize(inverse(ubo.model) * norm).xyz;
    oNormal = normalize(norm).xyz;

    // output world position of the vertex
    oWorldPos = world_pos.xyz;

    // output texture coordinates
    oTex = vTex;
}
