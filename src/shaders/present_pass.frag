
#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform PostProcessSettings
{
	bool Enable;
}ppSettings;

layout(set = 0, binding = 1) uniform sampler2D renderedScene;

// Reference: Implementation and Learnings based on:
// https://www.geeks3d.com/20101029/shader-library-pixelation-post-processing-effect-glsl/

void main()
{
    vec3 color = texture(renderedScene, uv).rgb;
    fragColor = vec4(color, 1.0);
}