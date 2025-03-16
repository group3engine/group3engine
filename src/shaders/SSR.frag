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
    int BinarySearchIterations;
    float MaxDistance;
    float thickness;
	float StepSize;
	float time;
}ssr;

layout (set = 0, binding = 1) uniform sampler2D depthBuffer;
layout (set = 0, binding = 3) uniform sampler2D renderedScene;
layout (set = 0, binding = 4) uniform sampler2D MetallicRoughness; // r = metallic, g = roughness
layout (set = 0, binding = 5) uniform samplerCube skybox;

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

vec3 BSWorld(vec3 worldRayPos, vec3 worldRayDir)
{
	for(int i = 0; i < ssr.BinarySearchIterations; i++)
	{
		// to screen-space
		vec4 projectedCoords = ubo.projection * ubo.view * vec4(worldRayPos, 1.0);
		projectedCoords.xyz /= projectedCoords.w;
		projectedCoords.xy = projectedCoords.xy * 0.5 + 0.5;

		float rayDepth = (projectedCoords.z);
		float depth = (texture(depthBuffer, projectedCoords.xy).x);

		float delta = rayDepth - depth;

		if(delta > 0.0)
			worldRayPos -= worldRayDir;
		else
			worldRayPos += worldRayDir;

		worldRayDir *= 0.5;
	}

	return worldRayPos;
}


vec3 worldToScreen(vec3 world)
{
	vec4 projected = ubo.projection * ubo.view * vec4(world, 1.0);
	projected = projected / projected.w; // perspective divide
	projected.xy = projected.xy * 0.5 + 0.5;

	return vec3(projected.xyz);
}

vec3 BinarySearch(vec3 raypos, vec3 raydir)
{
	for(int i = 0; i < ssr.BinarySearchIterations; i++)
	{
		float depth = texture(depthBuffer, raypos.xy).r;
		float depthDelta = depth - raypos.z;

		if(depthDelta > 0.0) raypos += raydir;
		else raypos -= raydir;


		raydir *= 0.5;
	}

	return raypos;
}

vec3 ViewToScreen(vec3 view)
{
	vec4 projected = ubo.projection * vec4(view, 1.0);
	projected = projected / projected.w; // perspective divide
	projected.xy = projected.xy * 0.5 + 0.5;

	return vec3(projected.xyz);
}

bool inScreenSpace(vec2 ssPos)
{
	if((ssPos.x >= 0.0 && ssPos.x <= 1.0 && ssPos.y >= 0.0 && ssPos.y <= 1.0))
	{
		return true;
	}

	return false;
}

float LinearizeDepth(float d)
{
    float zNear = ubo.nearPlane;
    float zFar = ubo.farPlane;
	return (zFar * zNear) / (zFar - zNear) / (d + zFar / (zNear - zFar));
}


vec4 NaiveScreenSpaceReflections()
{
	int STEPS = ssr.MaxSteps;
	float MAX_DISTANCE = ssr.MaxDistance;
	float stepSize = MAX_DISTANCE / float(STEPS);

	vec4 WorldPos = inverse(ubo.view) * vec4(DepthToPosition(uv).xyz, 1.0);
	vec4 WorldNormal = inverse(ubo.view) * vec4(DepthToNormal(uv).rgb, 0.0);

	vec3 camDir = normalize(WorldPos.xyz - ubo.cameraPosition.xyz);
	//vec3 reflectionDirection = sampleGGXVNDF(WorldNormal, ssr.thickness, getRandomXi(gl_FragCoord.xy));

	vec3 worldReflectionDir = normalize(reflect(camDir, WorldNormal.xyz));

	vec3 RayPos = WorldPos.xyz;
	vec3 RayStep = worldReflectionDir * stepSize;


	vec4 color = vec4(0,0,0,0);
	for(int i = 0; i < STEPS; i++)
	{
		//RayPos += ( i + noise(uv + ssr.time)) * RayStep;
		RayPos += RayStep * IGN(gl_FragCoord.xy);

		// Get the position of the ray in screen-space
		vec4 projectedCoords = ubo.projection * ubo.view * vec4(RayPos.xyz, 1.0);
		projectedCoords.xyz /= projectedCoords.w;
		projectedCoords.xy = projectedCoords.xy * 0.5 + 0.5;

		// check if outside view-frustum
		if(projectedCoords.x < 0.0 || projectedCoords.x > 1.0 || projectedCoords.y < 0.0 || projectedCoords.y > 1.0)
		{
			// fade out based on how far along the ray we are
			float fade = smoothstep(0.0, 1.0, float(i) * stepSize / MAX_DISTANCE);
			return vec4(0.0, 0.0, 0.0, 1.0);
		}

		float rayDepth = (projectedCoords.z);
		float depth = (texture(depthBuffer, projectedCoords.xy).x);

		if((rayDepth - depth) > 0.0 && (rayDepth - depth) < ssr.thickness)
		{
			// We hit geometry

			vec3 hitPos = RayPos;
			vec3 prevPos = RayPos - RayStep;
			for (int j = 0; j < ssr.BinarySearchIterations; j++) { // 4 iterations for refinement
				vec3 midPos = (hitPos + prevPos) * 0.5;
				vec4 midCoords = ubo.projection * ubo.view * vec4(midPos, 1.0);
				midCoords.xyz /= midCoords.w;
				midCoords.xy = midCoords.xy * 0.5 + 0.5;
				float midDepth = texture(depthBuffer, midCoords.xy).x;
				if (midDepth < midCoords.z) {
					hitPos = midPos;
				} else {
					prevPos = midPos;
				}
			}
			projectedCoords = ubo.projection * ubo.view * vec4(hitPos, 1.0);
			projectedCoords.xyz /= projectedCoords.w;
			projectedCoords.xy = projectedCoords.xy * 0.5 + 0.5;

			float NdotR = max(dot(-camDir, worldReflectionDir), 0.0);
			color = texture(renderedScene, projectedCoords.xy);

			vec2 center = vec2(0.5, 0.5);
			float fadeStart = 0.3;
			float fadeEnd = 0.5;

			float dist = length(projectedCoords.xy - center);

			//float fadeFactor = smoothstep(fadeEnd, fadeStart, dist);

			// Convert to NDC (-1 to 1 range)
			vec2 hitPixelNDC = projectedCoords.xy * 2.0 - 1.0;
			const float blendScreenEdgeFade = 5.0f;

			// Compute edge vignette (similar to CalculateEdgeVignette)
			vec2 vignette = clamp(abs(hitPixelNDC) * blendScreenEdgeFade - (blendScreenEdgeFade - 1.0), 0.0, 1.0);
			float fadeFactor = clamp(1.0 - dot(vignette, vignette), 0.0, 1.0);

			//color = color * fadeFactor;
			//mix(color, vec4(0), NdotR)
			return color;

			//return mix(color.rgb, vec3(0,0,0), NdotR); // if its closer to 1, we get less reflection since its aligned with camera
		}

		RayPos += RayStep;
	}

	// TODO: Read from cube map

	//vec4 skyboxColour = texture(skybox, worldReflectionDir);

	return vec4(0,0,0,1);
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
