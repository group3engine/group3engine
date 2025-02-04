#version 450

layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec2 iTexCoord;
layout (set = 0, binding = 0) uniform UScene
{
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
} uScene;

layout (location = 0) out vec2 v2fTexCoord;

void main()
{
    v2fTexCoord = iTexCoord;
    gl_Position = uScene.viewProjection * vec4(iPosition,  1.f);
}
