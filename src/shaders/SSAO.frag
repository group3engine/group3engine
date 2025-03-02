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


layout(set = 0, binding = 2) uniform SSAOSettings
{
	int NumDirections;
	int NumSteps;
	float Radius;
	float StepSize;
	float intensity;
}ssao;

layout (set = 0, binding = 1) uniform sampler2D depthBuffer;

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

#define PI 3.14159265359

vec2 snapToTexelCenter(vec2 texCoord, vec2 texSize) {
    vec2 texelSize = 1.0 / texSize;
    return floor(texCoord / texelSize) * texelSize + texelSize * 0.5;
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}


vec2 randomVector(vec2 uv) {
    float angle = hash(uv) * 6.2831853;
    return vec2(cos(angle), sin(angle));
}

vec4 generateNoise(vec2 uv) {
    vec2 rand2D = randomVector(uv);
    return vec4(rand2D, 0.0, 1.0);
}

vec2 RotateDirectionAngle(vec2 direction, vec2 noise)
{
    // contruct a rotation matrix to rotate the direction
    mat2 rotationMatrix = mat2(vec2(noise.x, -noise.y), vec2(noise.y, noise.x));
    return direction * rotationMatrix;
}

float SSAO()
{
	const int SAMPLING_DIRECTIONS = ssao.NumDirections;
	const float NUM_STEPS = ssao.NumSteps;
	const float STEP = ssao.StepSize;
	const float RADIUS = ssao.Radius;

	float occlusion = 0.0;
	float ao = 0.0;

	vec4 view_normals  = normalize(DepthToNormal(uv));
	view_normals.y = -view_normals.y;

	vec3 ssPos = vec3(uv, texture(depthBuffer, uv).x);
	vec3 ndc_pos = vec3(2.0 * ssPos.xy - 1.0, ssPos.z);

	vec4 unprojectedPosition = inverse(ubo.projection) * vec4(ndc_pos, 1.0);
	vec3 view_position = unprojectedPosition.xyz /= unprojectedPosition.w;

	float samplingDisk = 2 * PI / SAMPLING_DIRECTIONS;
	vec4 Rand = generateNoise(gl_FragCoord.xy * 0.01); // This needs to change to some actual noise

	for(int i = 0; i < SAMPLING_DIRECTIONS; i++)
	{
		float samplingAngle = i * samplingDisk;
		vec2 samplingDirection = RotateDirectionAngle(vec2(cos(samplingAngle), sin(samplingAngle)), Rand.xy);

		float tan_bias = tan(30.0 * PI / 180.0);
		float tangent_angle = acos(dot(vec3(samplingDirection, 0.0), view_normals.xyz)) - (0.5 * PI) + tan_bias;
		float horizon_angle = tangent_angle;

		vec3 last_difference = vec3(0);

		for(int j = 0; j < NUM_STEPS; j++)
		{
			vec2 step_screen_space = uv + (Rand.z + float(j + 1)) * STEP * samplingDirection;

			step_screen_space = snapToTexelCenter(step_screen_space, ubo.viewportSize);

			float stepped_location_z = texture(depthBuffer, step_screen_space.xy).x;
			vec3 stepped_location_position = vec3(step_screen_space, stepped_location_z);

			vec3 stepped_position_ndc = vec3(2.0 * stepped_location_position.xy - 1.0, stepped_location_position.z);
			vec4 stepped_position_unproj = inverse(ubo.projection) * vec4(stepped_position_ndc, 1.0);
			vec3 view_space_stepped_position = vec3(stepped_position_unproj.xyz / stepped_position_unproj.w);

			vec3 diff = view_space_stepped_position.xyz - view_position.xyz;

			if(length(diff) < RADIUS)
			{
				last_difference = diff;

				float elevation_angle = atan(diff.z / length(diff.xy));
				horizon_angle = max(horizon_angle, elevation_angle);
			}
		}

		float norm = length(last_difference) / RADIUS;
		float attenuation = 1 - (norm * norm);

		occlusion = clamp(attenuation * (sin(horizon_angle) - sin(tangent_angle)), 0.0, 1.0);
		ao += occlusion;
	}

	ao /= SAMPLING_DIRECTIONS;
	return 1.0 - ao * ssao.intensity;
}

void main()
{

	vec4 position = DepthToPosition(uv);
	vec4 normals = DepthToNormal(uv);

	float ao = SSAO();
	fragColour = vec4(vec3(ao), 1);
}
