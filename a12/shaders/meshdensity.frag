#version 450

layout(location = 0) flat in vec4 vertexPosition[3];
layout(location = 3) in vec4 interpolatedPosition;


layout(set = 4, binding = 0) uniform ScreenSize
{
    vec2 screenSize;
} screen;


layout (location = 0) out vec4 oColour;


// a list of colours to choose from when colouring the current triangle
vec3 colors[20] = vec3[](
vec3(1.0, 0.0, 0.0), // Red
vec3(0.0, 1.0, 0.0), // Green
vec3(0.0, 0.0, 1.0), // Blue
vec3(1.0, 1.0, 0.0), // Yellow
vec3(1.0, 0.0, 1.0), // Magenta
vec3(0.0, 1.0, 1.0), // Cyan
vec3(1.0, 0.5, 0.0), // Orange
vec3(0.5, 0.0, 1.0), // Purple
vec3(0.0, 0.5, 1.0), // Sky Blue
vec3(0.5, 1.0, 0.0), // Lime
vec3(1.0, 0.0, 0.5), // Pink
vec3(0.0, 1.0, 0.5), // Sea Green
vec3(0.5, 0.5, 0.5), // Grey
vec3(1.0, 0.75, 0.8), // Light Pink
vec3(0.6, 0.4, 0.2), // Brown
vec3(0.8, 0.8, 0.8), // Light Grey
vec3(0.2, 0.2, 0.2), // Dark Grey
vec3(0.1, 0.1, 0.1), // very dark grey
vec3(0.9, 1.0, 1.0), // almost white
vec3(0.9, 0.1, 0.1)// Crimson
);

vec2 screenSize = vec2(1280.0, 720.0);

float simpleRand(uint seed)
{
    return fract(sin(seed) * 1928.3792387);
}



void main()
{


    vec4 colour = vec4(0);
    // check if the triangle density is higher than 1 per pixel
    // this will occur if any of the vertices are within the pixel
    // if so, colour the pixel white
    int numVerticesInPixel = 0;
    for(int i = 0; i < 3; i++)
    {
        // get the difference between the vertex position and the interpolated position in pixels
        vec2 diff = (vertexPosition[i].xy / vertexPosition[i].w - interpolatedPosition.xy / interpolatedPosition.w) * screen.screenSize;
        if(diff.x >= 0 && diff.x < 1 && diff.y >= 0 && diff.y < 1)
        {
            // the vertex is in the pixel
            numVerticesInPixel++;
        }
    }
    // if there is 1 vertex in the pixel, then it probably could be considered a single triangle - there will be multiple triangles but it will only be a small portion of each triangle
    // if there are 2 or 3, then it is definitely more than one triangle
    if(numVerticesInPixel > 0)
    {
        colour = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else
    {


    // output the triangle id as a colour
    uint triangleID = gl_PrimitiveID;
    triangleID %= 255;
    colour = vec4(float(triangleID) / 255.0, 0.0, 0.0, 0.0);
    }

    oColour = colour;  

}
