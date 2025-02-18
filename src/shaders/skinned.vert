#version 450

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

layout(set = 2, binding = 0) uniform JointBuffer
{
	mat4 jointTransforms[256];
} jointBuffer;

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
}pc;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 joints;
layout(location = 4) in vec4 weights;

layout(location = 0) out vec4 WorldPos;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 WorldNormal;

// source: https://www.shadertoy.com/view/3s33zj
mat3 adjugate( in mat4 m )
{
	return mat3(cross(m[1].xyz, m[2].xyz),
	cross(m[2].xyz, m[0].xyz),
	cross(m[0].xyz, m[1].xyz));
}

void main()
{
	WorldNormal = normalize(adjugate(pc.ModelMatrix) * normal);
	uv = tex;
	// calculate the skinned transform
	mat4 skinnedTransform = jointBuffer.jointTransforms[int(joints.x)] * weights.x +
		jointBuffer.jointTransforms[int(joints.y)] * weights.y +
		jointBuffer.jointTransforms[int(joints.z)] * weights.z +
		jointBuffer.jointTransforms[int(joints.w)] * weights.w;
	// calculate the skinned position
	WorldPos = pc.ModelMatrix * skinnedTransform * vec4(pos, 1.0);
	gl_Position = ubo.projection * ubo.view * pc.ModelMatrix * vec4(pos, 1.0);
}
