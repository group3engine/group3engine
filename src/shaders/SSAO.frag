#version 450

#include "uniforms.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColour;

layout(set = 0, binding = 0) uniform block {
    CameraUBO ubo;
};

layout(set = 1, binding = 1) uniform SSAOSettings
{
	int NumDirections;
	int NumSteps;
	float Radius;
	float StepSize;
	float intensity;
}ssao;


layout (set = 1, binding = 0) uniform sampler2D depthBuffer;
layout (set = 1, binding = 2) uniform sampler2D normalRoughness;
layout (set = 1, binding = 3) uniform sampler2D NoiseTexture;

#define PI 3.14159265359

vec4 DepthToPosition(vec2 uv, float depth)
{
	vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewSpace = inverse(ubo.projection) * clipSpace;
	viewSpace.xyz /= viewSpace.w;

	return vec4(viewSpace.xyz, 1.0);
}

vec4 DepthToNormal(vec2 uv, float depth)
{
	vec4 viewPosition = DepthToPosition(uv, depth);
	vec3 n = (cross(dFdx(viewPosition.xyz), dFdy(viewPosition.xyz)));
	n *= -1;
	return vec4(normalize(n), 1.0);
}


vec2 snapToTexelCenter(vec2 texCoord, vec2 texSize) {
    vec2 texelSize = 1.0 / texSize;
    return floor(texCoord / texelSize) * texelSize + texelSize * 0.5;
}

vec2 RotateDirectionAngle(vec2 direction, vec2 noise) {
    float x = direction.x * noise.x - direction.y * noise.y;
    float y = direction.x * noise.y + direction.y * noise.x;
    return vec2(x, y);
}

vec4 GetJitter()
{
	return texture(NoiseTexture, (gl_FragCoord.xy / 4.0f));
}

float NUMBER_OF_SAMPLING_DIRECTIONS = ssao.NumDirections;
float STEP = ssao.StepSize; //0.04
float NUMBER_OF_STEPS = ssao.NumSteps;
float TANGENT_BIAS = 0.523599;
float HalfPI = 0.5 * PI;
float TAU = 2 * PI;
float RADIUS = ssao.Radius;
float RADIUS2 = RADIUS * RADIUS;
float invNumDirections = 1.0 / NUMBER_OF_SAMPLING_DIRECTIONS;

void main()
{
	float ao = 0.0;
	float occlusion = 0.0;

	float fragmentDepth = texture(depthBuffer, uv).x;
    vec4 normal = DepthToNormal(uv, fragmentDepth);
	normal.y = -normal.y;
    vec3 viewPosition = DepthToPosition(uv, fragmentDepth).xyz;

    float samplingDiskDirection = TAU / NUMBER_OF_SAMPLING_DIRECTIONS;
    vec4 Rand = GetJitter();

	// vec3(0,0,0) is camera position in view space
	// float centerDepth = distance(vec3(0,0,0), viewPosition.xyz);

    for(int i = 0; i < NUMBER_OF_SAMPLING_DIRECTIONS; i++) {

        float samplingDirectionAngle = i * samplingDiskDirection;
        vec2 samplingDirection = RotateDirectionAngle(vec2(cos(samplingDirectionAngle), sin(samplingDirectionAngle)), Rand.xy);

        float tangentAngle = acos(dot(vec3(samplingDirection, 0.0), normal.xyz)) - (HalfPI) + TANGENT_BIAS;
        float horizonAngle = tangentAngle;

        vec3 LastDifference = vec3(0);
        for(int j = 0; j < NUMBER_OF_STEPS; j++){

            vec2 stepForward = (Rand.z + float(j+1)) * STEP * samplingDirection;
            vec2 stepPosition = uv + stepForward;
			stepPosition = snapToTexelCenter(stepPosition, ubo.viewportSize);

			float depthAtPosition = texture(depthBuffer, stepPosition).x;
			vec3 sampleViewPos = DepthToPosition(stepPosition, depthAtPosition).xyz;
			vec3 diff = sampleViewPos - viewPosition;

			float diffLenghSq = dot(diff, diff);

			bool inRadius = diffLenghSq < RADIUS2;
			horizonAngle = inRadius ? max(horizonAngle, atan(diff.z / length(diff.xy))) : horizonAngle;
			LastDifference = inRadius ? diff : LastDifference;
        }

        float norm = length(LastDifference) / RADIUS;
        float attenuation = 1.0 - (norm * norm);
		float sinHorizon = sin(horizonAngle);
		float sinTangent = sin(tangentAngle);

        occlusion = clamp(attenuation * (sinHorizon - sinTangent), 0.0, 1.0);
        ao += 1.0 - occlusion * ssao.intensity;
    }

    ao *= invNumDirections;
    fragColour = vec4(vec3(ao), 1.0);
}
