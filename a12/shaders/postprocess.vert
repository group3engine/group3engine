#version 450

// define a full screen triangle
const vec2 vertices[3] = vec2[3](
    vec2(-1.0f, -1.0f),
    vec2( 3.0f, -1.0f),
    vec2(-1.0f,  3.0f)
);
// define the texture coordinates
const vec2 texCoords[3] = vec2[3](
    vec2(0.0f, 0.0f),
    vec2(2.0f, 0.0f),
    vec2(0.0f, 2.0f)
);

layout (location = 0) out vec2 v2fTexCoord;

void main()
{
    v2fTexCoord = texCoords[gl_VertexIndex];
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
}