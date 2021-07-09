#if @reverseZ
uniform float linearFac;
#endif

float getLinearDepth(in float z, in float viewZ)
{
#if @reverseZ
    return linearFac*viewZ;
#else
    return z;
#endif
}