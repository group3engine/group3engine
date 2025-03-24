
#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D renderedScene;
layout(set = 0, binding = 1) uniform sampler2D bloomPass;
layout(set = 0, binding = 2) uniform sampler2D SSAO;
layout(set = 0, binding = 3) uniform sampler2D SSR;

float SpatialDenoisedSSAO()
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

    return float(totalao / 16.0);
}

// This kind of works right now, needs changing
vec3 SpatialDenoisedSSR(vec2 uv)
{
    vec2 texelSize = 1.0 / textureSize(SSR, 0);
    vec2 tex = clamp(uv - texelSize * 2.0, texelSize * 2.0, 1.0 - texelSize * 2.0);

    // Gather for R, G, B separately
    vec4 g1r = textureGather(SSR, tex, 0);
    vec4 g1g = textureGather(SSR, tex, 1);
    vec4 g1b = textureGather(SSR, tex, 2);

    vec4 g2r = textureGather(SSR, tex + vec2(texelSize.x * 2.0, 0.0), 0);
    vec4 g2g = textureGather(SSR, tex + vec2(texelSize.x * 2.0, 0.0), 1);
    vec4 g2b = textureGather(SSR, tex + vec2(texelSize.x * 2.0, 0.0), 2);

    vec4 g3r = textureGather(SSR, tex + vec2(0.0, texelSize.y * 2.0), 0);
    vec4 g3g = textureGather(SSR, tex + vec2(0.0, texelSize.y * 2.0), 1);
    vec4 g3b = textureGather(SSR, tex + vec2(0.0, texelSize.y * 2.0), 2);

    vec4 g4r = textureGather(SSR, tex + vec2(texelSize.x * 2.0, texelSize.y * 2.0), 0);
    vec4 g4g = textureGather(SSR, tex + vec2(texelSize.x * 2.0, texelSize.y * 2.0), 1);
    vec4 g4b = textureGather(SSR, tex + vec2(texelSize.x * 2.0, texelSize.y * 2.0), 2);

    // Compute average across all gathered samples
    vec3 result = vec3(
        (g1r.r + g1r.g + g1r.b + g1r.a +
         g2r.r + g2r.g + g2r.b + g2r.a +
         g3r.r + g3r.g + g3r.b + g3r.a +
         g4r.r + g4r.g + g4r.b + g4r.a) / 16.0,

        (g1g.r + g1g.g + g1g.b + g1g.a +
         g2g.r + g2g.g + g2g.b + g2g.a +
         g3g.r + g3g.g + g3g.b + g3g.a +
         g4g.r + g4g.g + g4g.b + g4g.a) / 16.0,

        (g1b.r + g1b.g + g1b.b + g1b.a +
         g2b.r + g2b.g + g2b.b + g2b.a +
         g3b.r + g3b.g + g3b.b + g3b.a +
         g4b.r + g4b.g + g4b.b + g4b.a) / 16.0
    );

    return result;
}


// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESToneMappingFilm(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	vec4 lighting = texture(renderedScene, uv);
	vec4 bloom = texture(bloomPass, uv);
	float ssao = SpatialDenoisedSSAO();
	//vec3 ssr = SpatialDenoisedSSR(uv);

	vec3 hdrColor = (lighting.rgb) * ssao;
    hdrColor = hdrColor + bloom.rgb;
	//vec3 ldrColor = hdrColor / (hdrColor + vec3(1.0));

    vec3 ldrColor = ACESToneMappingFilm(hdrColor);
	vec3 result = ldrColor;
	vec3 gammaCorrectedColor = pow(result, vec3(1.0 / 2.2));

	fragColor = vec4(vec3(gammaCorrectedColor), 1.0);
}