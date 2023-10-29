#ifndef LIB_PARTICLE_SOFT
#define LIB_PARTICLE_SOFT

#include "lib/util/quickstep.glsl"

float viewDepth(float depth, float near, float far)
{
#if @reverseZ
    depth = 1.0 - depth;
#endif
    return (near * far) / ((far - near) * depth - far);
}

float calcSoftParticleFade(
    in vec3 viewDir,
    in vec3 viewPos,
    in vec3 viewNormal,
    float near,
    float far,
    float depth,
    float size,
    bool fade,
    float softFalloffDepth
    )
{
    float euclidianDepth = length(viewPos);

    const float falloffMultiplier = 0.33;
    const float contrast = 1.30;

    float sceneDepth = viewDepth(depth, near, far);
    float particleDepth = viewPos.z;
    float falloff = size * falloffMultiplier;
    float delta = particleDepth - sceneDepth;

    float viewBias = 1.0;

    if (fade)
    {
        float VdotN = dot(viewDir, viewNormal);
        viewBias = abs(VdotN) * quickstep(euclidianDepth / softFalloffDepth) * (1.0 - pow(1.0 - abs(VdotN), 1.3));
    }

    const float shift = 0.845;
    return shift * pow(clamp(delta/falloff, 0.0, 1.0), contrast) * viewBias;
}

#endif
