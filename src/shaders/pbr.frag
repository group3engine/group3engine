#version 450

#include "pbr_temp.glsl"

void main()
{
    vec4 colour = ComputeLighting();
    // apply alpha masking for bloom
    // if any of the colour channels are above 1.0, set alpha to 1.0
    if(max(max(colour.r, colour.g), colour.b) > 1.0)
    {
        colour.a = 1.0;
    }
    // otherwise, set the alpha to zero
    else
    {
        colour.a = 0.0;
    }
    oColour = colour;

}
