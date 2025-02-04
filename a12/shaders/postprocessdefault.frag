#version 450

layout (location = 0) in vec2 v2fTexCoord;

// colour texture
layout (set = 0, binding = 0) uniform sampler2D uTextureColour;

// output colour
layout (location = 0) out vec4 oColour;

void main()
{
    oColour = texture(uTextureColour, v2fTexCoord);
}