// define the maximum number of lights
#define MAX_LIGHTS 32
#define MAX_SHADOWS 4
#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_DIRECTIONAL_SHADOWS 4




vec3 BRDF(float roughness, float metalness, vec3 baseColour, vec3 surfaceNormal, vec3 lightDirection, vec3 viewDirection, vec3 halfVector)
{
    // Ldiffuse + (beckmann distribution * fresnel * masking) / (4 * clamped(dot(N, V)) * clamped(dot(N, L))


    // get the denominator first since its simplest
    float denominator = max(4.f * max(dot(surfaceNormal, viewDirection), 0.f) * max(dot(surfaceNormal, lightDirection), 0.f), 0.001f);

    // get the fresnel term
    vec3 fresnel0 = (1 - metalness) * vec3(0.04) + metalness * baseColour;
    vec3 fresnel = fresnel0 + (1 - fresnel0) * pow(1 - max(dot(halfVector, viewDirection), 0.f), 5.f);
    // get the diffuse term
    vec3 diffuse = (baseColour / 3.14159) * (vec3(1) - fresnel) * (1 - metalness);
    // get the normal distribution term (beckmann)
    float ndoth = max(dot(surfaceNormal, halfVector), 0.f);
    float normalDistribution = exp((ndoth * ndoth - 1) / max((roughness * roughness * ndoth * ndoth), 0.001f)) / max((3.14159 * roughness * roughness * ndoth * ndoth * ndoth * ndoth), 0.001f);
    // get the masking term (cook-torrance)
    float masking = min(1, min((2 * ndoth * max(dot(surfaceNormal, viewDirection), 0.f)) / max(dot(viewDirection, halfVector), 0.001f), (2 * ndoth * max(dot(surfaceNormal, lightDirection), 0.f)) / max(dot(viewDirection, halfVector), 0.001f)));


    return diffuse + (normalDistribution * fresnel * masking) / denominator;

}

#define ComputeAllLighting(worldPosition, viewDirection, albedo, roughness, metalness, normal, lightPosition, lightColour, totalLight) \
{ \
        vec3 lightDirection = lightPosition.xyz - worldPosition; \
        float distanceToLight = length(lightDirection); \
        lightDirection = normalize(lightDirection); \
        vec3 halfVector = normalize(viewDirection + lightDirection); \
        float falloffMultiplier = 1.0 / (distanceToLight * distanceToLight); \
        totalLight += BRDF(roughness, metalness, albedo, normal, lightDirection, viewDirection, halfVector) * \
                      lightColour.rgb * max(dot(normal, lightDirection), 0.0) * falloffMultiplier; \
 }
#define ComputeAllLightingWithShadows(worldPosition, viewDirection, albedo, roughness, metalness, normal, lightPosition, lightColour, totalLight, inShadow) \
{ \
        vec3 lightDirection = lightPosition.xyz - worldPosition; \
        float distanceToLight = length(lightDirection); \
        lightDirection = normalize(lightDirection); \
        vec3 halfVector = normalize(viewDirection + lightDirection); \
        float falloffMultiplier = 1.0 / (distanceToLight * distanceToLight); \
        totalLight += BRDF(roughness, metalness, albedo, normal, lightDirection, viewDirection, halfVector) * \
                      lightColour.rgb * max(dot(normal, lightDirection), 0.0) * falloffMultiplier * inShadow; \
 }
#define ComputeAllLightingDirectional(worldPosition, viewDirection, albedo, roughness, metalness, normal, lightDirection, lightColour, totalLight) \
{ \
        vec3 halfVector = normalize(viewDirection + lightDirection); \
        totalLight += BRDF(roughness, metalness, albedo, normal, lightDirection, viewDirection, halfVector) * \
                      lightColour.rgb * max(dot(normal, lightDirection), 0.0); \
 }
#define ComputeAllLightingDirectionalWithShadows(worldPosition, viewDirection, albedo, roughness, metalness, normal, lightDirection, lightColour, totalLight, inShadow) \
{ \
        vec3 halfVector = normalize(viewDirection + lightDirection); \
        totalLight += BRDF(roughness, metalness, albedo, normal, lightDirection, viewDirection, halfVector) * \
                      lightColour.rgb * max(dot(normal, lightDirection), 0.0) * inShadow; \
 }