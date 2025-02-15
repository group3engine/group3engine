#version 450

// turn on alpha masking for pbr then use it
#define ALPHA
#include "pbr_forward.glsl"


void main()
{
    oColour = ComputeLighting();
}