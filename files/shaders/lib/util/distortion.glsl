#ifndef LIB_UTIL_DISTORTION
#define LIB_UTIL_DISTORTION

vec4 applyDistortion(in vec4 color, in float strength, in float pixelDepth, in float sceneDepth)
{
    vec4 distortion = color;
    float invOcclusion = 1.0;

    // TODO: Investigate me. Alpha-clipping is enabled for refraction for what seems an arbitrary threshold, even when
    // there are no associated NIF properties.
    if (distortion.a < 0.1)
        discard;

    distortion.b = 0.0;

#if @reverseZ
    if (pixelDepth < sceneDepth)
#else
    if (pixelDepth > sceneDepth)
#endif
    {
        invOcclusion = 0.0;
        distortion.b = 1.0;
    }
    distortion.rg = color.rg * 2.0 - 1.0;

    distortion.rg *= invOcclusion * strength * color.a;

    return distortion;
}

#endif
