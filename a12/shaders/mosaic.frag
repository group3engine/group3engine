#version 450

layout (location = 0) in vec2 v2fTexCoord;

// colour texture
layout (set = 0, binding = 0) uniform sampler2D uTextureColour;

// output colour
layout (location = 0) out vec4 oColour;

void main()
{
    float x, y;
    // calculate the screen width and height from the frag coord and the texture coordinate
    if(v2fTexCoord.x == 0)
    {
        x = 0;
    }
    else
    {
        float screenWidth = gl_FragCoord.x / v2fTexCoord.x;
        // round down the x value to the nearest multiple of 5
        x = gl_FragCoord.x - int(gl_FragCoord.x) % 5;
        x = x / screenWidth;
    }
    if(v2fTexCoord.y == 0)
    {
        y = 0;
    }
    else
    {
        float screenHeight = gl_FragCoord.y / v2fTexCoord.y;
        // round down the y value to the nearest multiple of 3
        y = gl_FragCoord.y - int(gl_FragCoord.y) % 3;
        y = y / screenHeight;
    }
    oColour = texture(uTextureColour, vec2(x, y));
}