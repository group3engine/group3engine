#version 450

layout(location = 0) in vec3 iNormal;
layout(location = 1) in vec3 iWorldPos;
layout(location = 2) in vec2 iTex;
layout(location = 4) in vec4 iColor;

layout(location = 0) out vec4 oColor;

void main()
{
    oColor = vec4(0, 1, 0, 1);
}
