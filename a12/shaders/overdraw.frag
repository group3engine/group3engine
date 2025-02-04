#version 450

layout (location = 0) out vec4 oColour;


void main()
{
    // output the colour as increasing brightness by 1/20th for each overdraw
    oColour = vec4(0.05f, 0.05f, 0.05f, 1.f);
}
