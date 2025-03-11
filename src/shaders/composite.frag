
#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D renderedScene;
layout(set = 0, binding = 1) uniform sampler2D bloomPass;
layout(set = 0, binding = 2) uniform sampler2D SSAO;
layout(set = 0, binding = 3) uniform sampler2D SSR;

vec3 SpatialDenoisedSSAO()
{
    vec2 texelSize = 1.0 / textureSize(SSAO, 0);
    vec2 tex = clamp(uv - texelSize * 2.0, texelSize * 2.0, 1.0 - texelSize * 2.0);

    // textureGather will gather 4 looksups in a single call (r is ssao)
    vec4 g1 = textureGather(SSAO, tex, 0);
    vec4 g2 = textureGather(SSAO, tex + vec2(texelSize.x * 2.0, 0.0), 0);
    vec4 g3 = textureGather(SSAO, tex + vec2(0.0, texelSize.y * 2.0), 0);
    vec4 g4 = textureGather(SSAO, tex + vec2(texelSize.x * 2.0, texelSize.y * 2.0), 0);

    float totalao = g1.r + g1.g + g1.b + g1.a +
                    g2.r + g2.g + g2.b + g2.a +
                    g3.r + g3.g + g3.b + g3.a +
                    g4.r + g4.g + g4.b + g4.a;

    return vec3(totalao / 16.0);
}

void main()
{
	vec4 lighting = texture(renderedScene, uv);
	vec4 bloom = texture(bloomPass, uv);
	vec3 ssao = SpatialDenoisedSSAO();
	//vec4 ssr = texture(SSR, uv);

	vec3 hdrColor = vec3(lighting.rgb) * ssao.r;
	vec3 ldrColor = hdrColor / (hdrColor + vec3(1.0));

	vec3 result = ldrColor;
	vec3 gammaCorrectedColor = pow(result, vec3(1.0 / 2.2));

	fragColor = vec4(vec3(hdrColor), 1.0);
}