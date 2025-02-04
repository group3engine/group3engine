
mat3 quaternionToMatrix(vec4 q)
{
    return mat3(
    1.0f - (2.0f * (q.y * q.y + q.z * q.z)), 2.0f * (q.x * q.y + q.w * q.z), 2.0f * (q.x * q.z - q.w * q.y),
    2.0f * (q.x * q.y - q.w * q.z), 1.0f - (2.0f * (q.x * q.x + q.z * q.z)), 2.0f * (q.y * q.z + q.w * q.x),
    2.0f * (q.x * q.z + q.w * q.y), 2.0f * (q.y * q.z - q.w * q.x), 1.0f - (2.0f * (q.x * q.x + q.y * q.y))
);
}

