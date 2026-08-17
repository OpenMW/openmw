#include "lib/light/struct.glsl"

#if @skyBlending
#include "lib/core/fragment.h.glsl"

uniform float skyBlendingStart;
#endif

struct Fog {
    vec4 color;
    vec4 underwaterColor;
    float start;
    float underwaterStart;
    float end;
    float underwaterEnd;
    float depth;
};

uniform Fog fog;
uniform float waterHeight;
uniform bool waterEnabled;
uniform bool isReflection;
uniform mat4 osg_ViewMatrixInverse;
uniform DirectionalLight sun;

const vec3 WATER_COLOR = vec3(0.090195, 0.115685, 0.12745);

vec4 applyFogAtDist(vec4 color, vec3 pos, float euclideanDist, float linearDist, float near, float far)
{
#if @radialFog
    float dist = euclideanDist;
#else
    float dist = abs(linearDist);
#endif

    bool isUnderwater = false;
    bool useWaterDepthFog = false;
    float underwaterFogFactor = 1.0;

    if (waterEnabled) {
        vec3 cameraPos = osg_ViewMatrixInverse[3].xyz;
        bool cameraBelowWater = cameraPos.z < waterHeight;
        if (cameraBelowWater) {
            isUnderwater = true;
        } else {
            vec3 worldPos = (osg_ViewMatrixInverse * vec4(pos, 1)).xyz;
            const float bias = 5.0;

            if (!isReflection)
                isUnderwater = worldPos.z < waterHeight - bias;

            if (isUnderwater) {
                useWaterDepthFog = true;
                const float visibility = 2500.0;
                const float depthFade = 0.15;
                float waterDepth = dist * clamp((waterHeight - worldPos.z) / (cameraPos.z - worldPos.z), 0.0, 1.0);
                float depthCorrection = sqrt(1.0 + 4.0 * depthFade * depthFade);
                underwaterFogFactor = depthFade * depthFade
                    / (-0.5 * depthCorrection + 0.5 - waterDepth / visibility) + 0.5 * depthCorrection + 0.5;
                underwaterFogFactor = clamp(underwaterFogFactor, 0.0, 1.0);
            }
        }
    }

    vec4 fogColor = isUnderwater ? fog.underwaterColor : fog.color;
    float start = isUnderwater ? fog.underwaterStart : fog.start;
    float end = isUnderwater ? fog.underwaterEnd : fog.end;

    if (fog.depth >= 0.0) {
        start = near * fog.depth + far * (1.0 - fog.depth);
        end = far;
        useWaterDepthFog = false;
    }

    if (useWaterDepthFog)
        fogColor.rgb = WATER_COLOR * length(sun.ambient.xyz);

#if @exponentialFog
    float fogValue = 1.0 - exp(-2.0 * max(0.0, dist - start / 2.0) / (end - start / 2.0));
#else
    float fogValue = clamp((dist - start) * (1.0 / (end - start)), 0.0, 1.0);
#endif
    if (useWaterDepthFog)
        fogValue = underwaterFogFactor;

#ifdef ADDITIVE_BLENDING
    color.xyz *= 1.0 - fogValue;
#else
    color.xyz = mix(color.xyz, fogColor.xyz, fogValue);
#endif

#if @skyBlending
if (!isUnderwater && !isReflection) {
    float fadeValue = clamp((far - dist) / (far - skyBlendingStart), 0.0, 1.0);
    fadeValue *= fadeValue;
#ifdef ADDITIVE_BLENDING
    color.xyz *= fadeValue;
#else
    color.xyz = mix(sampleSkyColor(gl_FragCoord.xy / screenRes), color.xyz, fadeValue);
#endif
}
#endif

    return color;
}

vec4 applyFogAtPos(vec4 color, vec3 pos, float near, float far)
{
    return applyFogAtDist(color, pos, length(pos), pos.z, near, far);
}
