
// given an input uint32_t, decompress it into a vec4 quaternion
// the 2 most significant bits are the index of the largest component
// the other 30 bits are the 10 bits of each component
vec4 decompressQuaternion(uint compressed)
{
    // extract the index of the largest component
    uint index = compressed >> 30;
    // extract the 10 bits of each component
    uint x = (compressed >> 20) & 0x3FFu;
    uint y = (compressed >> 10) & 0x3FFu;
    uint z = compressed & 0x3FFu;
    // convert the 10 bits to a float in the range [0, 1]
    float fx = float(x) / 1023.0f;
    float fy = float(y) / 1023.0f;
    float fz = float(z) / 1023.0f;
    // convert the float to the range [-1/sqrt(2), 1/sqrt(2)]
    float oneOverSqrt2 = 1.0f / sqrt(2.0f);
    fx = fx * 2.f * oneOverSqrt2 - oneOverSqrt2;
    fy = fy * 2.f * oneOverSqrt2 - oneOverSqrt2;
    fz = fz * 2.f * oneOverSqrt2 - oneOverSqrt2;
    // calculate the largest component
    float fw = sqrt(1.0f - fx * fx - fy * fy - fz * fz);
    // create the quaternion
    switch (index)
    {
        case 0:
            return vec4(fw, fx, fy, fz);
        case 1:
            return vec4(fx, fw, fy, fz);
        case 2:
            return vec4(fx, fy, fw, fz);
        case 3:
            return vec4(fx, fy, fz, fw);
    }
    return vec4(0.f);
}