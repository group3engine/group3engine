#version 450

// NOTE: Some data when 2 player was implemented has broken something
// Works with a single player but stopped working correctly when two players was introduced

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 0) uniform CameraUBO
{
	mat4 view;
	mat4 projection;
	mat4 inverseProjection;
	mat4 inverseView;
    vec4 cameraPosition;
    vec2 viewportSize;
	float fov;
	float nearPlane;
	float farPlane;
} ubo;


layout(set = 1, binding = 1) uniform SSRSettings
{
    int MaxSteps;
    int BinarySearchIterations;
    float MaxDistance;
    float thickness;
	float StepSize;
	float time;
}ssr;

layout (set = 1, binding = 0) uniform sampler2D depthBuffer;
layout (set = 1, binding = 2) uniform sampler2D renderedScene;
layout (set = 1, binding = 3) uniform sampler2D normalRoughness; // rgb = normals, a = roughness
layout (set = 1, binding = 4) uniform samplerCube skybox;

vec4 DepthToPosition(vec2 uv)
{
	float depth = texture(depthBuffer, uv).x;
	vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewSpace = inverse(ubo.projection) * clipSpace;
	viewSpace.xyz /= viewSpace.w;

	return vec4(viewSpace.xyz, 1.0);
}

// This should be replaced storing normals in the render target during thin g-buffer creation.
// This one is not accurate enough.
vec4 DepthToNormal(vec2 uv)
{
	float depth = texture(depthBuffer, uv).x;

	vec4 viewPosition = DepthToPosition(uv);

	vec3 n = normalize(cross(dFdx(viewPosition.xyz), dFdy(viewPosition.xyz)));
	n *= -1;

	return vec4(n, 1.0);
}

// This should be faster and better quality than original implementation
vec3 ScreenSpaceReflections()
{
    const vec2 texSize = textureSize(normalRoughness, 0);
    const float MAX_DISTANCE = ssr.MaxDistance;

    vec4 WorldPos = inverse(ubo.view) * vec4(DepthToPosition(uv).xyz, 1.0);
    vec3 camDir = normalize(WorldPos.xyz - ubo.cameraPosition.xyz);
    vec4 WorldNormal = texture(normalRoughness, uv); //(xyz is normal, .a is roughness)
    WorldNormal.xyz = normalize(WorldNormal.xyz * 2.0 - 1.0);

    //vec4 WorldNormal = normalize(inverse(ubo.view) * vec4(DepthToNormal(uv).xyz, 0.0)); // Depth for normals can cause issues when looking straight down. Store normals in a g-buffer target instead
    vec3 worldReflectionDir = normalize(reflect(camDir, WorldNormal.xyz));

    vec3 WorldSpaceBegin = WorldPos.xyz;
    vec3 worldSpaceEnd = WorldSpaceBegin + worldReflectionDir * MAX_DISTANCE;

    // project to screen space
    vec4 start = ubo.projection * ubo.view * vec4(WorldSpaceBegin, 1.0);
    start.xyz /= start.w;
    start.xy = start.xy * 0.5 + 0.5;
    start.xy *= texSize;

    vec4 end = ubo.projection * ubo.view * vec4(worldSpaceEnd, 1.0);
    end.xyz /= end.w;
    end.xy = end.xy * 0.5 + 0.5;
    end.xy *= texSize;

    // get the step direction, take largest to prevent branching
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    int stepDir = max(abs(int(dx)), abs(int(dy)));

	// early exit if start and end are the same we dont need to traverse the ray
    if (stepDir == 0) {
        return vec3(0.0);
    }

    float stepRCP = 1.0 / float(stepDir);
    float x_incr = dx * stepRCP;
    float y_incr = dy * stepRCP;

    float x = start.x;
    float y = start.y;

    // move along the ray
    for (int i = 0; i < stepDir; i++) {

        x += x_incr;
        y += y_incr;

        // Check if x, y are outside the bounds of pixel-space-screen-space
        if (x < 0.0 || x >= texSize.x || y < 0.0 || y >= texSize.y) {
            break;
        }

        // interpolate depth to find depth at current position
        float t = float(i) / float(stepDir);
        float z = mix(start.z, end.z, t);
        float depth = texelFetch(depthBuffer, ivec2(x, y), 0).x;

        // depth test to determine hit
        float depthDiff = z - depth;
        if (depthDiff > 0.0 && depthDiff < ssr.thickness) {

			// sample the colour at the hit point
			vec3 colour = texelFetch(renderedScene, ivec2(x, y), 0).rgb;

			// screen-fading at edges
			vec2 hitPixelNDC = (vec2(x,y) / texSize) * 2.0 - 1.0; // get in NDC [-1, 1]
			const float blendScreenEdgeFade = 2.0f; // TODO: Make tweakable parameter?
			vec2 vignette = clamp(abs(hitPixelNDC) * blendScreenEdgeFade - (blendScreenEdgeFade - 1.0), 0.0, 1.0);
			float screenFade = clamp(1.0 - dot(vignette, vignette), 0.0, 1.0);

			// rays which point towards the camera have less contribution (likely hitting the back of a surface)
			float NdotR = max(dot(normalize(-camDir), (worldReflectionDir)), 0.0);
            float TowardsCameraVisibility = (1 - clamp(NdotR, 0.0, 1.0));

			// fade the ray based on the distance the ray has travelled
			float DistanceTravelled = (1.0 - clamp(float(i) / float(stepDir), 0.0, 1.0));
			//return colour * TowardsCameraVisibility * DistanceTravelled * screenFade;
            return colour;
        }
    }

    // TODO: Sample cube map if no intersection

    return vec3(0.0);
}

void main()
{
    fragColour = vec4(ScreenSpaceReflections().xyz, 1.0);
	//fragColour = vec4(mix(ScreenSpaceReflections().rgb, vec3(0), roughness), 1.0);
}
