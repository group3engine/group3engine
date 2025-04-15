// SH basis functions
float SH00() { return 0.282095; }
float SH1m1(vec3 v) { return 0.488603 * v.y; }
float SH10(vec3 v) { return 0.488603 * v.z; }
float SH11(vec3 v) { return 0.488603 * v.x; }
float SH2m2(vec3 v) { return 1.092548 * v.x * v.y; }
float SH2m1(vec3 v) { return 1.092548 * v.y * v.z; }
float SH20(vec3 v) { return 0.315392 * (3.0 * v.z * v.z - 1.0); }
float SH21(vec3 v) { return 1.092548 * v.x * v.z; }
float SH22(vec3 v) { return 0.546274 * (v.x * v.x - v.y * v.y); }

// source: https://www.shadertoy.com/view/3s33zj
mat3 adjugate( in mat4 m )
{
    return mat3(cross(m[1].xyz, m[2].xyz),
                cross(m[2].xyz, m[0].xyz),
                cross(m[0].xyz, m[1].xyz));
}

mat3 tbn(vec3 normal, vec4 tangent, mat4 modelMatrix) {
    mat3 normalMatrix = adjugate(modelMatrix);
    vec3 normalW = normalize(normalMatrix * normal);
    vec3 tangentW = normalize(mat3(modelMatrix) * tangent.xyz);
    vec3 bitangentW = cross(normalW, tangentW) * tangent.w;
    return mat3(tangentW, bitangentW, normalW);
}
