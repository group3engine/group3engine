
#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D renderedScene;
layout(set = 0, binding = 1) uniform sampler2D bloomPass;
layout(set = 0, binding = 2) uniform sampler2D SSAO;
layout(set = 0, binding = 3) uniform sampler2D SSR;
layout(set = 0, binding = 4) uniform sampler2D Fog;


layout(set = 0, binding = 5) uniform PostProcessSettings
{
    float brightness;
    float contrast;
    float saturation;
    int toneMap;
}ppSettings;

layout (set = 0, binding = 6) uniform RendererDebug
{
    int debugMode;
}debugRenderer;

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

    // Gather for R, G, B separately - TextureGather does single values
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

vec3 SD(sampler2D inputImage)
{
    ivec2 loc = ivec2(gl_FragCoord.xy) - ivec2(2);
	vec3 total = vec3(0.0);

    vec2 texelSize = 1.0 / vec2(textureSize(inputImage, 0));
    vec3 result = vec3(0.0);
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(inputImage, uv + offset).rgb;
        }
    }

    return vec3(result / 16.0);
}

vec3 ACESFilm(vec3 x){
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 Saturation(vec3 color, float saturation) {
    vec3 gScale = vec3(0.299, 0.587, 0.114);
    vec3 grayscale = vec3(dot(color, gScale));
    vec3 result = mix(grayscale, color, saturation);
    return clamp(result, 0.0, 1.0);
}
vec3 ContrastBrightness(vec3 color, float contrast, float brightness) {
    return clamp(contrast * (color - vec3(0.5)) + vec3(0.5) + brightness, 0.0, 1.0);
}

vec3 Reinhard(vec3 color)
{
    float luminance = dot(color, vec3(0.299f, 0.587f, 0.114f));
    float reinhard = luminance / (luminance + 1.0f);
    return color * (reinhard / luminance);
}

// https://www.shadertoy.com/view/lslGzl
vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	return color;
}

void main()
{
	vec4 lighting = texture(renderedScene, uv);
	vec4 bloom = texture(bloomPass, uv);
	vec3 ssao = SD(SSAO);
    vec3 ssr = texture(SSR, uv).rgb;
    vec4 FoggedScene = texture(Fog, uv);

    // The fog is now composed with the final lighting
    vec3 compositeFog = mix(lighting.rgb, FoggedScene.rgb, FoggedScene.a).rgb;

    // FoggedScene is now just "lighting".
    // With fog = 0, its just the scene.
	vec3 hdrColor = (compositeFog.rgb + ssr) * ssao;
    hdrColor = Saturation(hdrColor, ppSettings.saturation);
    hdrColor = ContrastBrightness(hdrColor, ppSettings.contrast, ppSettings.brightness);

    hdrColor = hdrColor + bloom.rgb; 
    vec3 ldrColor = ppSettings.toneMap == 0 ? hdrColor : ppSettings.toneMap == 1 ? Reinhard(hdrColor) : ppSettings.toneMap == 2 ? Uncharted2ToneMapping(hdrColor) : ACESFilm(hdrColor);
	vec3 result = ldrColor;

    // Check if we need to apply the post process combinatio since 1 - 7 is debug modes
    bool applyProcessing = !(debugRenderer.debugMode >= 1 && debugRenderer.debugMode <= 8);
    // Since lighting = forwardPass where debug is also handled, determine if full processing is needed or not
    vec3 gammaCorrectedColor = applyProcessing ? pow(result, vec3(1.0 / 2.2)) : lighting.rgb;

    switch(debugRenderer.debugMode)
    {
        case 9:
            fragColor = vec4(ssao, 1.0);
            break;
        case 10:
            fragColor = vec4(ssr, 1.0);
            break;
        default:
            fragColor = vec4(gammaCorrectedColor, 1.0);
    }

	//fragColor = vec4(vec3(gammaCorrectedColor), 1.0);
}