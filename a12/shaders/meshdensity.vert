#version 450

layout (location = 0) in vec3 iPosition;

layout (set = 0, binding = 0) uniform UScene
{
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
} uScene;

layout(location = 0) flat out vec4 vertexPosition[3];
layout (location = 3) out vec4 interpolatedPosition;


void main()
{
    gl_Position = uScene.viewProjection * vec4(iPosition,  1.f);
    vertexPosition[gl_VertexIndex % 3] = uScene.viewProjection * vec4(iPosition,  1.f);
    interpolatedPosition = uScene.viewProjection * vec4(iPosition,  1.f);
}
