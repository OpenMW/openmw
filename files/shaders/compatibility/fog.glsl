#if @skyBlending
#include "lib/core/fragment.h.glsl"

uniform float skyBlendingStart;
#endif

struct Fog {
    vec4 color;
    float start;
    float end;
};

uniform Fog fog;

vec4 applyFogAtDist(vec4 color, float euclideanDist, float linearDist, float far)
{
#if @radialFog
    float dist = euclideanDist;
#else
    float dist = abs(linearDist);
#endif
#if @exponentialFog
    float fogValue = 1.0 - exp(-2.0 * max(0.0, dist - fog.start / 2.0) / (fog.end - fog.start / 2.0));
#else
    float fogValue = clamp((dist - fog.start) * (1.0 / (fog.end - fog.start)), 0.0, 1.0);
#endif
#ifdef ADDITIVE_BLENDING
    color.xyz *= 1.0 - fogValue;
#else
    color.xyz = mix(color.xyz, fog.color.xyz, fogValue);
#endif

#if @skyBlending
    float fadeValue = clamp((far - dist) / (far - skyBlendingStart), 0.0, 1.0);
    fadeValue *= fadeValue;
#ifdef ADDITIVE_BLENDING
    color.xyz *= fadeValue;
#else
    color.xyz = mix(sampleSkyColor(gl_FragCoord.xy / screenRes), color.xyz, fadeValue);
#endif
#endif

    return color;
}

vec4 applyFogAtPos(vec4 color, vec3 pos, float far)
{
    return applyFogAtDist(color, length(pos), pos.z, far);
}
