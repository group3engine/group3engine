#version 450

layout (location = 0) in vec2 v2fTexCoord;

// colour texture
layout (set = 1, binding = 0) uniform sampler2D uTextureColour;
// roughness texture
layout (set = 1, binding = 1) uniform sampler2D uTextureRoughness;
// metalness texture
layout (set = 1, binding = 2) uniform sampler2D uTextureMetalness;
// alpha mask texture
layout (set = 1, binding = 3) uniform sampler2D uTextureAlphaMask;
// normal texture
layout (set = 1, binding = 4) uniform sampler2D uTextureNormal;
// emissive texture
layout (set = 1, binding = 5) uniform sampler2D uTextureEmissive;


layout (location = 0) out vec4 oColour;


void main()
{
    oColour = vec4(texture(uTextureColour, v2fTexCoord).rgb, 1.f);
}
