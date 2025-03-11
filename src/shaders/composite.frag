
#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D renderedScene;
layout(set = 0, binding = 1) uniform sampler2D bloomPass;
layout(set = 0, binding = 2) uniform sampler2D SSAO;
layout(set = 0, binding = 3) uniform sampler2D SSR;

void main()
{
	vec4 lighting = texture(renderedScene, uv);
	vec4 bloom = texture(bloomPass, uv);
	vec4 ssao = texture(SSAO, uv);
	//vec4 ssr = texture(SSR, uv);

	vec3 hdrColor = (lighting.rgb) * ssao.r;
	vec3 ldrColor = hdrColor / (hdrColor + vec3(1.0));

	vec3 result = ldrColor;
	vec3 gammaCorrectedColor = pow(result, vec3(1.0 / 2.2));

	fragColor = vec4(vec3(gammaCorrectedColor), 1.0);
}