#version 450

#include "uniforms.glsl"

layout(location = 0) in vec3 texCoords;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

layout(set = 1, binding = 0) uniform samplerCube cubemap;

void main()
{
    fragColor = texture(cubemap, texCoords);
}