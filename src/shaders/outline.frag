#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColour;

layout (set = 0, binding = 0) uniform sampler2D depthBuffer;
layout (set = 0, binding = 1) uniform sampler2D normalRoughness;
layout (set = 0, binding = 2) uniform outlineSettings
{
    int outlineWidth;
    float sobelThreshold;
}outline;



void main()
{
    // we are going to do 4 sobel passes: normal horizontal, normal vertical, depth horizontal, depth vertical
    float finalSobel = 0.0;
    // sample the 8 surrounding pixels of the depth buffer
    vec2 depthTexelSize = 1.0 / textureSize(depthBuffer, 0);
    vec2 normalTexelSize = 1.0 / textureSize(normalRoughness, 0);
    float depthTopLeft = texture(depthBuffer, uv - vec2(depthTexelSize.x, depthTexelSize.y)).r;
    float depthTopRight = texture(depthBuffer, uv + vec2(depthTexelSize.x, -depthTexelSize.y)).r;
    float depthBottomLeft = texture(depthBuffer, uv - vec2(-depthTexelSize.x, depthTexelSize.y)).r;
    float depthBottomRight = texture(depthBuffer, uv + vec2(-depthTexelSize.x, -depthTexelSize.y)).r;
    float depthTop = texture(depthBuffer, uv + vec2(0.0, -depthTexelSize.y)).r;
    float depthBottom = texture(depthBuffer, uv + vec2(0.0, depthTexelSize.y)).r;
    float depthLeft = texture(depthBuffer, uv - vec2(depthTexelSize.x, 0.0)).r;
    float depthRight = texture(depthBuffer, uv + vec2(-depthTexelSize.x, 0.0)).r;
    float depthHorizontal = depthTopLeft * -1.0 + depthLeft * -2.0 + depthBottomLeft * -1.0 + depthTopRight * 1.0 + depthRight * 2.0 + depthBottomRight * 1.0;
    depthHorizontal = abs(depthHorizontal);
    float depthVertical = depthTopLeft * -1.0 + depthTop * -2.0 + depthTopRight * -1.0 + depthBottomLeft * 1.0 + depthBottom * 2.0 + depthBottomRight * 1.0;
    depthVertical = abs(depthVertical);
    float depthEdge = depthHorizontal + depthVertical;
    // Sample neighboring normals
    vec3 normalTopLeft = texture(normalRoughness, uv - vec2(normalTexelSize.x, normalTexelSize.y)).xyz;
    vec3 normalTopRight = texture(normalRoughness, uv + vec2(normalTexelSize.x, -normalTexelSize.y)).xyz;
    vec3 normalBottomLeft = texture(normalRoughness, uv - vec2(-normalTexelSize.x, normalTexelSize.y)).xyz;
    vec3 normalBottomRight = texture(normalRoughness, uv + vec2(-normalTexelSize.x, -normalTexelSize.y)).xyz;
    vec3 normalTop = texture(normalRoughness, uv + vec2(0.0, -normalTexelSize.y)).xyz;
    vec3 normalBottom = texture(normalRoughness, uv + vec2(0.0, normalTexelSize.y)).xyz;
    vec3 normalLeft = texture(normalRoughness, uv - vec2(normalTexelSize.x, 0.0)).xyz;
    vec3 normalRight = texture(normalRoughness, uv + vec2(-normalTexelSize.x, 0.0)).xyz;
    float normalHorizontalX = normalTopLeft.x * -1.0 + normalLeft.x * -2.0 + normalBottomLeft.x * -1.0 + normalTopRight.x * 1.0 + normalRight.x * 2.0 + normalBottomRight.x * 1.0;
    float normalHorizontalY = normalTopLeft.y * -1.0 + normalLeft.y * -2.0 + normalBottomLeft.y * -1.0 + normalTopRight.y * 1.0 + normalRight.y * 2.0 + normalBottomRight.y * 1.0;
    float normalHorizontalZ = normalTopLeft.z * -1.0 + normalLeft.z * -2.0 + normalBottomLeft.z * -1.0 + normalTopRight.z * 1.0 + normalRight.z * 2.0 + normalBottomRight.z * 1.0;
    float normalVerticalX = normalTopLeft.x * -1.0 + normalTop.x * -2.0 + normalTopRight.x * -1.0 + normalBottomLeft.x * 1.0 + normalBottom.x * 2.0 + normalBottomRight.x * 1.0;
    float normalVerticalY = normalTopLeft.y * -1.0 + normalTop.y * -2.0 + normalTopRight.y * -1.0 + normalBottomLeft.y * 1.0 + normalBottom.y * 2.0 + normalBottomRight.y * 1.0;
    float normalVerticalZ = normalTopLeft.z * -1.0 + normalTop.z * -2.0 + normalTopRight.z * -1.0 + normalBottomLeft.z * 1.0 + normalBottom.z * 2.0 + normalBottomRight.z * 1.0;
    float normalHorizontal = abs(normalHorizontalX) + abs(normalHorizontalY) + abs(normalHorizontalZ);
    float normalVertical = abs(normalVerticalX) + abs(normalVerticalY) + abs(normalVerticalZ);
    normalHorizontal = normalHorizontal * 0.1; // reduce the intensity of the normal edge
    normalVertical = normalVertical * 0.1; // reduce the intensity of the normal edge


    float normalEdge = abs(normalHorizontal) + abs(normalVertical);

    finalSobel = max(depthEdge, normalEdge);
    // sobel threshold
    finalSobel = step(outline.sobelThreshold, finalSobel);

    fragColour = vec4(vec3(finalSobel), 1.0);


}