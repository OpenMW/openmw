#if @reverseZ
uniform float linearFac;
#endif

float getLinearDepth(in vec4 viewPos)
{
#if @reverseZ
    return linearFac*viewPos.z;
#else
    return gl_Position.z;
#endif
}