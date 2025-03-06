#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 0) uniform SceneUniform
{
	mat4 model;
	mat4 view;
	mat4 projection;
    vec4 cameraPosition;
    vec2 viewportSize;
	float fov;
	float nearPlane;
	float farPlane;
} ubo;


layout(set = 0, binding = 2) uniform SSRSettings
{
	int MaxSteps;
	int MaxDistance;
}ssr;

layout (set = 0, binding = 1) uniform sampler2D depthBuffer;
layout (set = 0, binding = 3) uniform sampler2D renderedScene;
layout (set = 0, binding = 4) uniform sampler2D MetallicRoughness; // r = metallic, g = roughness

vec4 DepthToPosition(vec2 uv)
{
	float depth = texture(depthBuffer, uv).x;
	vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewSpace = inverse(ubo.projection) * clipSpace;
	viewSpace.xyz /= viewSpace.w;

	return vec4(viewSpace.xyz, 1.0);
}

vec4 DepthToNormal(vec2 uv)
{
	float depth = texture(depthBuffer, uv).x;

	vec4 viewPosition = DepthToPosition(uv);

	vec3 n = normalize(cross(dFdx(viewPosition.xyz), dFdy(viewPosition.xyz)));
	n *= -1;

	return vec4(n, 1.0);
}

float IGN(vec2 p)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract( magic.z * fract(dot(p,magic.xy)) );
}

float noise(vec2 seed)
{
    return fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 NaiveScreenSpaceReflections()
{
	const vec2 texSize = textureSize(depthBuffer, 0);
	const vec2 uv = gl_FragCoord.xy / texSize;
	int STEPS = ssr.MaxSteps;
	int MAX_DISTANCE = ssr.MaxDistance;
	float stepSize = float(MAX_DISTANCE) / STEPS;

	vec4 WorldPos = inverse(ubo.view) * DepthToPosition(uv);
	vec4 WorldNormal = normalize(inverse(ubo.view) * vec4(DepthToNormal(uv).xyz, 0.0));

	vec3 camDir = normalize(WorldPos.xyz - ubo.cameraPosition.xyz);
	vec3 worldReflectionDir = normalize(reflect(camDir, WorldNormal.xyz));

	vec3 RayPos = WorldPos.xyz;
	vec3 RayStep = worldReflectionDir * stepSize;

	RayPos += RayStep;
	vec4 color = vec4(0,0,0,0);
	for(int i = 0; i < STEPS; i++)
	{
		// Get the position of the ray in screen-space
		vec4 projectedCoords = ubo.projection * ubo.view * vec4(RayPos.xyz, 1.0);
		projectedCoords.xyz /= projectedCoords.w;
		projectedCoords.xy = projectedCoords.xy * 0.5 + 0.5;

		// check if outside view-frustum
		if(projectedCoords.x < 0.0 || projectedCoords.x > 1.0 || projectedCoords.y < 0.0 || projectedCoords.y > 1.0)
		{
			// fade out based on how far along the ray we are
			float fade = smoothstep(0.0, 1.0, float(i) * stepSize / MAX_DISTANCE);
			return vec3(0.0, 0.0, 0.0) * fade;
		}

		float rayDepth = projectedCoords.z;
		float depth = texture(depthBuffer, projectedCoords.xy).x;

		if((rayDepth - depth) > 0.0 && (rayDepth - depth) < 0.1)
		{
			// We hit geometry
			float NdotR = max(dot(-camDir, worldReflectionDir), 0.0);
			color.rgb = texture(renderedScene, projectedCoords.xy).rgb;
			return mix(color.rgb, vec3(0,0,0), NdotR); // if its closer to 1, we get less reflection since its aligned with camera
		}

		RayPos += RayStep * (i + noise(uv));
	}

	return vec3(0.0, 0.0, 0.0);
}

void main()
{
	vec4 position = DepthToPosition(uv);
	vec4 normals = DepthToNormal(uv);

	float metallic = texture(MetallicRoughness, uv).r;
	float roughness = texture(MetallicRoughness, uv).g;


	fragColour = vec4(mix(vec3(0), NaiveScreenSpaceReflections().rgb, metallic), 1.0);

	//fragColour = vec4(vec3(NaiveScreenSpaceReflections().xyz), clamp(metallic, 0.0, 1.0));
}
