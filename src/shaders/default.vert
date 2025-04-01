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

layout(push_constant) uniform Push
{
	mat4 ModelMatrix;
}pc;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;

layout(location = 0) out vec4 WorldPos;
layout(location = 1) out vec2 uv;
layout(location = 2) out mat3 TBNFrame;

// source: https://www.shadertoy.com/view/3s33zj
mat3 adjugate( in mat4 m )
{
	return mat3(cross(m[1].xyz, m[2].xyz),
	cross(m[2].xyz, m[0].xyz),
	cross(m[0].xyz, m[1].xyz));
}

void main()
{
    vec3 biTangent = cross(normalize(normal), normalize(tangent.xyz)) * tangent.w;
    TBNFrame = adjugate(pc.ModelMatrix) * mat3(normalize(tangent.xyz), normalize(biTangent), normalize(normal));
	uv = tex;
	WorldPos = pc.ModelMatrix * vec4(pos, 1.0);
	gl_Position = ubo.projection * ubo.view * pc.ModelMatrix * vec4(pos, 1.0);
}
