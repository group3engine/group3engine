#version 450
#include "quaternion2matrix.glsl"
#include "decompressQuaternion.glsl"

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
} pushConstants;

layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec2 iTexCoord;
layout (location = 2) in vec3 iNormal;

layout (set = 0, binding = 0) uniform UScene
{
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
} uScene;

// source: https://www.shadertoy.com/view/3s33zj
mat3 adjugate( in mat4 m )
{
    return mat3(cross(m[1].xyz, m[2].xyz),
    cross(m[2].xyz, m[0].xyz),
    cross(m[0].xyz, m[1].xyz));
}

layout (location = 0) out vec2 v2fTexCoord;
layout (location = 1) out vec3 v2fPosition;
layout (location = 2) out vec3 v2fNormal;
void main()
{
    v2fTexCoord = iTexCoord;
    gl_Position = uScene.viewProjection * pushConstants.modelMatrix * vec4(iPosition,  1.f);
    vec4 worldSpacePosition = (pushConstants.modelMatrix * vec4(iPosition, 1.f));
    v2fPosition = worldSpacePosition.xyz / worldSpacePosition.w;
    v2fNormal = adjugate(pushConstants.modelMatrix) * iNormal;
}
