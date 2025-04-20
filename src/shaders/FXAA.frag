#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColour;

layout (set = 0, binding = 0) uniform sampler2D compositeImage;

layout (set = 0, binding = 1) uniform FXAASettings
{
	bool EnableFXAA;
}fxaa;

#ifndef FXAA_REDUCE_MIN
    #define FXAA_REDUCE_MIN (1.0 / 128.0)
#endif
#ifndef FXAA_REDUCE_MUL
    #define FXAA_REDUCE_MUL (1.0 / 8.0)
#endif
#ifndef FXAA_SPAN_MAX
    #define FXAA_SPAN_MAX 8.0
#endif

// Based on https://github.com/mattdesl/glsl-fxaa/blob/master/fxaa.glsl
vec4 FXAA(sampler2D tex, vec2 uv, vec2 resolution) {

    vec2 inverseVP = 1.0 / resolution;
    vec2 offsetNW = uv + vec2(-1.0, -1.0) * inverseVP;
    vec2 offsetNE = uv + vec2( 1.0, -1.0) * inverseVP;
    vec2 offsetSW = uv + vec2(-1.0,  1.0) * inverseVP;
    vec2 offsetSE = uv + vec2( 1.0,  1.0) * inverseVP;

    vec3 rgbNW = texture(tex, offsetNW).rgb;
    vec3 rgbNE = texture(tex, offsetNE).rgb;
    vec3 rgbSW = texture(tex, offsetSW).rgb;
    vec3 rgbSE = texture(tex, offsetSE).rgb;
    vec4 texColor = texture(tex, uv);
    vec3 rgbM  = texColor.rgb;

    vec3 lumaWeights = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, lumaWeights);
    float lumaNE = dot(rgbNE, lumaWeights);
    float lumaSW = dot(rgbSW, lumaWeights);
    float lumaSE = dot(rgbSE, lumaWeights);
    float lumaM  = dot(rgbM,  lumaWeights);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, -FXAA_SPAN_MAX, FXAA_SPAN_MAX) * inverseVP;

    vec3 rgbA = 0.5 * (texture(tex, uv + dir * (1.0 / 3.0 - 0.5)).rgb + texture(tex, uv + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(tex, uv + dir * -0.5).rgb + texture(tex, uv + dir *  0.5).rgb);

    float lumaB = dot(rgbB, lumaWeights);

    bool check = (lumaB < lumaMin || lumaB > lumaMax);
    return check ? vec4(rgbA, texColor.a) : vec4(rgbB, texColor.a);

}

void main()
{
    if (fxaa.EnableFXAA == true) {
        fragColour = vec4(FXAA(compositeImage, uv, textureSize(compositeImage, 0)).rgb, 1.0);
    }
    else {
        fragColour = vec4(texture(compositeImage, uv).rgb, 1.0);
    }
}
