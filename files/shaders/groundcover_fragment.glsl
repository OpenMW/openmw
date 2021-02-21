#version 120
#extension GL_ARB_uniform_buffer_object : enable

#define GROUNDCOVER

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

#if @normalMap
uniform sampler2D normalMap;
varying vec2 normalMapUV;
varying vec4 passTangent;
#endif

// Other shaders respect forcePPL, but legacy groundcover mods were designed to work with vertex lighting.
// They may do not look as intended with per-pixel lighting, so ignore this setting for now.
#define PER_PIXEL_LIGHTING @normalMap

varying float euclideanDepth;
varying float linearDepth;

#if PER_PIXEL_LIGHTING
varying vec3 passViewPos;
varying vec3 passNormal;
#else
centroid varying vec3 passLighting;
centroid varying vec3 shadowDiffuseLighting;
#endif

#include "shadows_fragment.glsl"
#include "lighting.glsl"

float calc_coverage(float a, float alpha_ref, float falloff_rate)
{
    return clamp(falloff_rate * (a - alpha_ref) + alpha_ref, 0.0, 1.0);
}

void main()
{
#if @normalMap
    vec4 normalTex = texture2D(normalMap, normalMapUV);

    vec3 normalizedNormal = normalize(passNormal);
    vec3 normalizedTangent = normalize(passTangent.xyz);
    vec3 binormal = cross(normalizedTangent, normalizedNormal) * passTangent.w;
    mat3 tbnTranspose = mat3(normalizedTangent, binormal, normalizedNormal);

    vec3 viewNormal = gl_NormalMatrix * normalize(tbnTranspose * (normalTex.xyz * 2.0 - 1.0));
#endif

#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV);
#else
    gl_FragData[0] = vec4(1.0);
#endif

    gl_FragData[0].a = calc_coverage(gl_FragData[0].a, 128.0/255.0, 4.0);

    float shadowing = unshadowedLightRatio(linearDepth);
    if (euclideanDepth > @groundcoverFadeStart)
        gl_FragData[0].a *= 1.0-smoothstep(@groundcoverFadeStart, @groundcoverFadeEnd, euclideanDepth);

    vec3 lighting;
#if !PER_PIXEL_LIGHTING
    lighting = passLighting + shadowDiffuseLighting * shadowing;
#else
    vec3 diffuseLight, ambientLight;
    doLighting(passViewPos, normalize(viewNormal), shadowing, diffuseLight, ambientLight);
    lighting = diffuseLight + ambientLight;
#endif

#if @clamp
    lighting = clamp(lighting, vec3(0.0), vec3(1.0));
#else
    lighting = max(lighting, 0.0);
#endif

    gl_FragData[0].xyz *= lighting;

#if @radialFog
    float fogValue = clamp((euclideanDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#else
    float fogValue = clamp((linearDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#endif
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, gl_Fog.color.xyz, fogValue);

    applyShadowDebugOverlay();
}
