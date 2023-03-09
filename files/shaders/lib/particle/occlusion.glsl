#ifndef LIB_PARTICLE_OCCLUSION
#define LIB_PARTICLE_OCCLUSION

void applyOcclusionDiscard(in vec3 coord, float sceneDepth)
{
#if @reverseZ
    if (coord.z < sceneDepth)
        discard;
#else
    if (coord.z * 0.5 + 0.5 > sceneDepth)
        discard;
#endif
}

#endif
