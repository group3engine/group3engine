#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 oColour;

layout(set = 0, binding = 0) uniform sampler2D uTextureColour;

float linearWeights[11] = float[11](0.04432692004460363, 0.08729996976964827, 0.08208967306397075, 0.07348225552094238, 0.06261752605291124, 0.05079590950246383, 0.03922662104658954, 0.028837146065052336, 0.020181003028350505, 0.013444732514627586, 0.00852668794435865);
float linearOffsets[11] = float[11](0.0, 1.4953705026712067, 3.489199211318705, 5.483031210517226, 7.476868375857014, 9.470712576642052, 11.464565673628174, 13.4584295167832, 15.452305943074823, 17.446196774291817, 19.440103814903964);

vec4 getColour(float x, float y)
{
    // divide by the width and height of the texture
    x = x / textureSize(uTextureColour, 0).x;
    y = y / textureSize(uTextureColour, 0).y;
    // sample the texture at that offset
    vec4 col =  texture(uTextureColour, vec2(x, y) + uv);
    return col;
}

void main()
{
    vec4 texColour = texture(uTextureColour, uv);
    // apply the horizontal blur
    vec4 blurredOutput = getColour(0.0, 0.0) * linearWeights[0];
    // blur horizontally
    for(int i = 1; i < 11; i++)
    {
        blurredOutput += getColour(linearOffsets[i] / 3., 0.0) * linearWeights[i];
        blurredOutput += getColour(-linearOffsets[i] / 3., 0.0) * linearWeights[i];
    }
    oColour = blurredOutput;
}
