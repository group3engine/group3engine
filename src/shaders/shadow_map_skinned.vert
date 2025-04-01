#version 450

struct Light
{
	int Type;
	vec4 LightPosition;
	vec4 LightColour;
	mat4 LightSpaceMatrix;
};

const int NUM_LIGHTS = 26;

layout(set = 0, binding = 0) uniform LightBuffer {
	Light lights[NUM_LIGHTS];
} lightData;

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
}pc;

layout(location = 0) in vec3 pos;
layout(location = 4) in vec4 joints;
layout(location = 5) in vec4 weights;


// source: https://www.shadertoy.com/view/3s33zj
mat3 adjugate( in mat4 m )
{
	return mat3(cross(m[1].xyz, m[2].xyz),
	cross(m[2].xyz, m[0].xyz),
	cross(m[0].xyz, m[1].xyz));
}
mat4 rotationX45 = mat4(
	1.0, 0.0,                 0.0,                0.0,
	0.0, cos(radians(45.0)), -sin(radians(45.0)), 0.0,
	0.0, sin(radians(45.0)),  cos(radians(45.0)), 0.0,
	0.0, 0.0,                 0.0,                1.0
);

layout(set = 2, binding = 0) uniform JointBuffer
{
	mat4 jointTransforms[256];
} jointBuffer;


void main()
{
	mat4 skinMat =
		weights.x * jointBuffer.jointTransforms[int(joints.x)] +
		weights.y * jointBuffer.jointTransforms[int(joints.y)] +
		weights.z * jointBuffer.jointTransforms[int(joints.z)] +
		weights.w * jointBuffer.jointTransforms[int(joints.w)];

	vec4 WorldPos = skinMat * vec4(pos, 1.0);
	gl_Position = lightData.lights[0].LightSpaceMatrix * vec4(WorldPos.xyz, 1.0);
}

