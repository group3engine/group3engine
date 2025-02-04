#version 450

layout (location = 0) in vec3 iPosition;

layout (set = 0, binding = 0) uniform UScene
{
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
} uScene;


void main()
{
    gl_Position = uScene.viewProjection * vec4(iPosition,  1.f);
}
