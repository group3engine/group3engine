#version 450



layout (location = 0) out vec4 oColour;


void main()
{
    // linearize the depth value
    // magic clip space values
    float zNear = 0.1;
    float zFar = 100.0;
    float z = gl_FragCoord.z;
    float linearDepth = zNear / (zFar + z * (zNear-zFar));
    // output the linear depth as a colour
    oColour = vec4(vec3(linearDepth), 1.f);
}
