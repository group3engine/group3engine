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
    float dzdx = dFdx(z) * 2000.f;
    float dzdy = dFdy(z) * 2000.f;

    oColour = vec4(abs(dzdx), abs(dzdy), 0.f, 1.f);
}
