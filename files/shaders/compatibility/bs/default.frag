#version 120
#pragma import_defines(FORCE_OPAQUE, DISTORTION, WRITE_NORMALS, FORCE_PPL, CLASSIC_FALLOFF, MAX_LIGHTS)

#define PER_PIXEL_LIGHTING 1

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

#if @emissiveMap
uniform sampler2D emissiveMap;
varying vec2 emissiveMapUV;
#endif

#if @normalMap
uniform sampler2D normalMap;
varying vec2 normalMapUV;
#endif

varying float euclideanDepth;
varying float linearDepth;

varying vec3 passViewPos;
varying vec3 passNormal;

uniform vec2 screenRes;
uniform float far;
uniform float alphaRef;
uniform float emissiveMult;
uniform float specStrength;
uniform bool useTreeAnim;
uniform float distortionStrength;

#include "lib/core/fragment.h.glsl"
#include "lib/light/lighting.glsl"
#include "lib/material/alpha.glsl"
#include "lib/util/distortion.glsl"

#include "compatibility/vertexcolors.glsl"
#include "compatibility/shadows_fragment.glsl"
#include "compatibility/fog.glsl"
#include "compatibility/normals.glsl"

void main()
{
#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV);

#if defined(DISTORTION) && DISTORTION
    vec2 screenCoords = gl_FragCoord.xy / (screenRes * @distorionRTRatio);
    gl_FragData[0].a *= getDiffuseColor().a;
    gl_FragData[0] = applyDistortion(gl_FragData[0], distortionStrength, gl_FragCoord.z, sampleOpaqueDepthTex(screenCoords).x);

    return;
#endif

    gl_FragData[0].a *= coveragePreservingAlphaScale(diffuseMap, diffuseMapUV);
#else
    gl_FragData[0] = vec4(1.0);
#endif

    vec4 diffuseColor = getDiffuseColor();
    if (!useTreeAnim)
        gl_FragData[0].a *= diffuseColor.a;
    gl_FragData[0].a = alphaTest(gl_FragData[0].a, alphaRef);

    vec3 specularColor = getSpecularColor().xyz;
#if @normalMap
    vec4 normalTex = texture2D(normalMap, normalMapUV);
    vec3 normal = normalTex.xyz * 2.0 - 1.0;
#if @reconstructNormalZ
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
#endif
    vec3 viewNormal = normalToView(normal);
    specularColor *= normalTex.a;
#else
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
#endif

    float shadowing = unshadowedLightRatio(linearDepth);
    vec3 diffuseLight, ambientLight, specularLight;
    doLighting(passViewPos, viewNormal, gl_FrontMaterial.shininess, shadowing, diffuseLight, ambientLight, specularLight);
    vec3 diffuse = diffuseColor.xyz * diffuseLight;
    vec3 ambient = getAmbientColor().xyz * ambientLight;
    vec3 emission = getEmissionColor().xyz * emissiveMult;
#if @emissiveMap
    emission *= texture2D(emissiveMap, emissiveMapUV).xyz;
#endif
    vec3 lighting = diffuse + ambient + emission;
    vec3 specular = specularColor * specularLight * specStrength;

    clampLightingResult(lighting);

    gl_FragData[0].xyz = gl_FragData[0].xyz * lighting + specular;

    gl_FragData[0] = applyFogAtDist(gl_FragData[0], euclideanDepth, linearDepth, far);

#if defined(FORCE_OPAQUE) && FORCE_OPAQUE
    // having testing & blending isn't enough - we need to write an opaque pixel to be opaque
    gl_FragData[0].a = 1.0;
#endif

#if !defined(FORCE_OPAQUE) && defined(WRITE_NORMALS) && WRITE_NORMALS
    gl_FragData[1].xyz = viewNormal * 0.5 + 0.5;
#endif

    applyShadowDebugOverlay();
}
