#version 450

#include "uniforms.glsl"

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec4 iColor;

layout(location = 0) out vec4 oColor;

void main() {
    gl_Position = ubo.projection * ubo.view * vec4(iPosition, 1.0);
    oColor = iColor;
}
