#version 450

layout (location = 0) in vec2 v2fTexCoord;

// colour texture
layout (set = 1, binding = 0) uniform sampler2D uTextureColour;


layout (location = 0) out vec4 oColour;


void main()
{
    // get the mipmap level of the texture
    float mipLevel = fract(textureQueryLod(uTextureColour, v2fTexCoord).y);
    // output the mip level as a colour
    oColour = vec4(vec3(mipLevel), 1.f);

}
