#version 450

layout (location = 0) in vec2 v2fTexCoord;

// colour texture
layout (set = 0, binding = 0) uniform sampler2D uTextureColour;

// depth texture
layout (set = 0, binding = 1) uniform sampler2D uTextureDepth;

// output colour
layout (location = 0) out vec4 oColour;

vec2 texCoord2PixelCoord(vec2 texCoord, vec2 textureSize)
{
    return texCoord * textureSize;
}

vec2 pixelCoord2TexCoord(vec2 pixelCoord, vec2 textureSize)
{
    return pixelCoord / textureSize;
}

float linearizeDepth(float depth)
{
    // magic clip space values
    float zNear = 0.1;
    float zFar = 100.0;
    return zNear / (zFar + depth * (zNear - zFar));
}

float depthAt(vec2 texCoord)
{
    return linearizeDepth(texture(uTextureDepth, texCoord).r);
}

int colourAt(vec2 texCoord)
{
    return int(texture(uTextureColour, texCoord).r * 255.0);
}

const int kernel[7][7] = int[7][7](
int[7](1, 1, 1, 1, 1, 1, 1),
int[7](1, 2, 2, 2, 2, 2, 1),
int[7](1, 2, 3, 3, 3, 2, 1),
int[7](1, 2, 3, 0, 3, 2, 1),
int[7](1, 2, 3, 3, 3, 2, 1),
int[7](1, 2, 2, 2, 2, 2, 1),
int[7](1, 1, 1, 1, 1, 1, 1)
);

vec3 baseColour = vec3(0.098, 0.961, 0.635);


void main()
{
    // if the texture has non zero green, set the colour to blue
    // its a greater than in case there's been anti-aliasing
    // the number it is greater than is fairly high in case the background clear colour is light
    if(texture(uTextureColour, v2fTexCoord).g > 0.8)
    {
        oColour = vec4(0.0, 0.0, 1.0, 1.0);
        return;
    }

    // get the size of the screen
    vec2 screenSize = vec2(textureSize(uTextureColour, 0));
    vec2 pixelCoord = texCoord2PixelCoord(v2fTexCoord, screenSize);
    float depthAtCenter = depthAt(v2fTexCoord);
    int numDifferentColours = 0;
    int alreadyChecked = colourAt(v2fTexCoord);
    for (int x = -3; x <= 3; x++)
    {
        for (int y = -3; y <= 3; y++)
        {
            if (x == 0 && y == 0)
            {
                continue;
            }
            vec2 offset = vec2(x, y);
            vec2 offsetTexCoord = pixelCoord2TexCoord(pixelCoord + offset, screenSize);
            float depthAtOffset = depthAt(offsetTexCoord);
            if (abs(depthAtOffset - depthAtCenter) < 0.01)
            {
                // if the colour is already checked in the already checked bit mask, skip this pixel
                int colour = colourAt(offsetTexCoord);
                int mask = 1 << colour;
                if((alreadyChecked & mask) == mask)
                {
                    continue;
                }
                alreadyChecked |= mask;
                numDifferentColours++;
            }

        }
    }
    if(numDifferentColours > 2)
    {
        oColour = vec4(baseColour * (numDifferentColours / 49.0), 1.f);
    }
    else
    {
        oColour = vec4(baseColour / 60.0, 1.f);
    }

}