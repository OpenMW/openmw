#ifndef LIB_VIEW_DEPTH
#define LIB_VIEW_DEPTH

float linearizeDepth(float depth, float near, float far)
{
#if @reverseZ
    depth = 1.0 - depth;
#endif
    float z_n = 2.0 * depth - 1.0;
    depth = 2.0 * near * far / (far + near - z_n * (far - near));
    return depth;
}

float getLinearDepth(in float z, in float viewZ)
{
#if @reverseZ
    // FIXME: Fixme, figure out how to calculate correct linear depth for reverse z
    return -viewZ;
#else
    return z;
#endif
}

#endif
