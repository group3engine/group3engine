#version 450

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
} pushConstants;

layout (location = 0) in vec3 iPosition;

layout (set = 0, binding = 0) uniform UScene
{
    mat4 vp;
} uScene;

void main()
{
    gl_Position = uScene.vp * pushConstants.modelMatrix * vec4(iPosition,  1.f);
}
