#if @skyBlending
#include "lib/core/fragment.h.glsl"

uniform float skyBlendingStart;
#endif

struct Fog {
    vec4 color;
    vec4 underWaterColor;
    float start;
    float underWaterStart;
    float end;
    float underWaterEnd;
    float depth;
};

uniform Fog fog;
uniform float waterHeight;
uniform bool waterEnabled;
uniform bool isReflection;
uniform mat4 osg_ViewMatrixInverse;

vec4 applyFogAtDist(vec4 color, vec3 pos, float euclideanDist, float linearDist, float near, float far)
{
#if @radialFog
    float dist = euclideanDist;
#else
    float dist = abs(linearDist);
#endif

    bool isUnderWater = false;

    float factor = 1.0;

    if (waterEnabled) {
        vec3 cameraPos = osg_ViewMatrixInverse[3].xyz;
        bool cameraBelowWater = cameraPos.z <= waterHeight;
        if (cameraBelowWater) {
            isUnderWater = true;
        } else {
            vec3 worldPos = (osg_ViewMatrixInverse * vec4(pos, 1)).xyz;
            float bias = (cameraBelowWater) ? -5.0 : 5.0;

            if (!isReflection)
                isUnderWater = worldPos.z < waterHeight - bias;
            else if (cameraPos.z < waterHeight)
                isUnderWater = worldPos.z > waterHeight + bias;

            if (isUnderWater) {
                float waterDepth = dist * clamp((waterHeight - worldPos.z) / (cameraPos.z - worldPos.z), 0.0, 1.0);
                const float DEPTH_FADE = 0.15;
                const float VISIBILITY = 2500.0;
                float depthCorrection = sqrt(1.0 + 4.0 * DEPTH_FADE * DEPTH_FADE);
                factor = DEPTH_FADE * DEPTH_FADE / (-0.5 * depthCorrection + 0.5 - waterDepth / VISIBILITY) + 0.5 * depthCorrection + 0.5;
                factor = clamp(factor, 0.0, 1.0);
            }
        }
    }

    vec4 fogColor = isUnderWater ? fog.underWaterColor : fog.color;
    float start = isUnderWater ? fog.underWaterStart : fog.start;
    float end = isUnderWater ? fog.underWaterEnd : fog.end;

    if (fog.depth >= 0.0) {
        start = near * fog.depth + far * (1.0 - fog.depth);
        end = far;
    }

#if @exponentialFog
    float fogValue = 1.0 - exp(-2.0 * max(0.0, dist - start / 2.0) / (end - start / 2.0));
#else
    float fogValue = clamp((dist - start) * (1.0 / (end - start)), 0.0, 1.0);
#endif

    fogValue *= factor;

#ifdef ADDITIVE_BLENDING
    color.xyz *= 1.0 - fogValue;
#else
    color.xyz = mix(color.xyz, fogColor.xyz, fogValue);
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

vec4 applyFogAtPos(vec4 color, vec3 pos, float near, float far)
{
    return applyFogAtDist(color, pos, length(pos), pos.z, near, far);
}
