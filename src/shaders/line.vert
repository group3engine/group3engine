#version 450

layout(set = 0, binding = 0) uniform SceneUniform {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
    vec2 viewportSize;
    float fov;
    float nearPlane;
    float farPlane;
} ubo;

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec4 iColor;

layout(location = 0) out vec4 oColor;

void main() {
    gl_Position = ubo.projection * ubo.view * vec4(iPosition, 1.0);
    oColor = iColor;
}
