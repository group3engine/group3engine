#version 450

struct Particle{
    mat4 transform;
    vec3 colour;
    int isEnabled;
};

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

layout (set = 2, binding = 0) readonly buffer ParticleUniform
{
    Particle particles[];
} pUBO;


layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;


layout(location = 0) out vec4 WorldPos;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 WorldNormal;

// source: https://www.shadertoy.com/view/3s33zj
mat3 adjugate( in mat4 m )
{
    return mat3(cross(m[1].xyz, m[2].xyz),
    cross(m[2].xyz, m[0].xyz),
    cross(m[0].xyz, m[1].xyz));
}


void main()
{
    // get the particle index from the instance id
    int particleIndex = gl_InstanceIndex;
    vec4 position;
    // if the particle isn't enabled, set gl_position to offscreen (-1)
    if (pUBO.particles[particleIndex].isEnabled == 0)
    {
        position = vec4(-1,-1,-1,1);
    }
    else
    {
        position = pUBO.particles[particleIndex].transform * vec4( pos, 1.0);
    }
    WorldNormal = normalize(adjugate(pUBO.particles[particleIndex].transform) * normal);
    uv = tex;
    WorldPos = position;
    gl_Position = ubo.projection * ubo.view * position;
}
