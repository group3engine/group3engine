// SH basis functions
float SH00() { return 0.282095; } // Y_0^0
float SH1m1(vec3 v) { return 0.488603 * v.y; } // Y_1^{-1}
float SH10(vec3 v) { return 0.488603 * v.z; } // Y_1^0
float SH11(vec3 v) { return 0.488603 * v.x; } // Y_1^1
float SH2m2(vec3 v) { return 1.092548 * v.y * v.x; } // Y_2^{-2}
float SH2m1(vec3 v) { return 0.546274 * v.y * v.z; } // Y_2^{-1}
float SH20(vec3 v) { return 0.315392 * (3.0 * v.z * v.z - 1.0); } // Y_2^0
float SH21(vec3 v) { return 0.546274 * v.x * v.z; } // Y_2^1
float SH22(vec3 v) { return 0.546274 * (v.x * v.x - v.y * v.y); } // Y_2^2

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
