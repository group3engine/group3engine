#version 450

layout(set = 0, binding = 0) uniform CameraUBO
{
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
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec4 joints;
layout(location = 5) in vec4 weights;

layout(location = 0) out vec4 WorldPos;
layout(location = 1) out vec2 uv;
layout(location = 2) out mat3 TBNFrame;
layout(location = 5) out vec4 WorldNormal;

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

void main()
{
	uv = tex;

	mat4 skinMat =
		weights.x * jointBuffer.jointTransforms[int(joints.x)] +
		weights.y * jointBuffer.jointTransforms[int(joints.y)] +
		weights.z * jointBuffer.jointTransforms[int(joints.z)] +
		weights.w * jointBuffer.jointTransforms[int(joints.w)];

	WorldPos = skinMat * vec4(pos, 1.0);
	gl_Position = ubo.projection * ubo.view * WorldPos;
	WorldNormal = pc.ModelMatrix * vec4(normal, 0.0);
    vec3 biTangent = cross(normalize(normal), normalize(tangent.xyz)) * tangent.w;
    TBNFrame = adjugate(skinMat) * mat3(normalize(tangent.xyz), normalize(biTangent), normalize(normal));
}
