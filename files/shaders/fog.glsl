uniform float far;

#if @skyBlending
#include "openmw_fragment.h.glsl"

uniform float skyBlendingStart;
#endif

vec4 applyFogAtDist(vec4 color, float euclideanDist, float linearDist)
{
#if @radialFog
    float dist = euclideanDist;
#else
    float dist = abs(linearDist);
#endif
#if @exponentialFog
    float fogValue = 1.0 - exp(-2.0 * max(0.0, dist - gl_Fog.start/2.0) / (gl_Fog.end - gl_Fog.start/2.0));
#else
    float fogValue = clamp((dist - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#endif
#ifdef ADDITIVE_BLENDING
    color.xyz *= 1.0 - fogValue;
#else
    color.xyz = mix(color.xyz, gl_Fog.color.xyz, fogValue);
#endif

#if @skyBlending
    float fadeValue = clamp((far - dist) / (far - skyBlendingStart), 0.0, 1.0);
    fadeValue *= fadeValue;
#ifdef ADDITIVE_BLENDING
    color.xyz *= fadeValue;
#else
    color.xyz = mix(mw_sampleSkyColor(gl_FragCoord.xy / screenRes), color.xyz, fadeValue);
#endif
#endif

    return color;
}

vec4 applyFogAtPos(vec4 color, vec3 pos)
{
    return applyFogAtDist(color, length(pos), pos.z);
}
