#version 120
#pragma import_defines(FORCE_OPAQUE)

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

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
varying vec4 passTangent;
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

#include "lib/light/lighting.glsl"
#include "lib/material/alpha.glsl"

#include "compatibility/vertexcolors.glsl"
#include "compatibility/shadows_fragment.glsl"
#include "compatibility/fog.glsl"

void main()
{
    vec3 normal = normalize(passNormal);

#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV);
    gl_FragData[0].a *= coveragePreservingAlphaScale(diffuseMap, diffuseMapUV);
#else
    gl_FragData[0] = vec4(1.0);
#endif

    vec4 diffuseColor = getDiffuseColor();
    gl_FragData[0].a *= diffuseColor.a;
    gl_FragData[0].a = alphaTest(gl_FragData[0].a, alphaRef);

#if @normalMap
    vec4 normalTex = texture2D(normalMap, normalMapUV);

    vec3 normalizedNormal = normal;
    vec3 normalizedTangent = normalize(passTangent.xyz);
    vec3 binormal = cross(normalizedTangent, normalizedNormal) * passTangent.w;
    mat3 tbnTranspose = mat3(normalizedTangent, binormal, normalizedNormal);

    normal = normalize(tbnTranspose * (normalTex.xyz * 2.0 - 1.0));
#endif
    vec3 viewNormal = normalize(gl_NormalMatrix * normal);

    float shadowing = unshadowedLightRatio(linearDepth);
    vec3 diffuseLight, ambientLight;
    doLighting(passViewPos, viewNormal, shadowing, diffuseLight, ambientLight);
    vec3 emission = getEmissionColor().xyz * emissiveMult;
#if @emissiveMap
    emission *= texture2D(emissiveMap, emissiveMapUV).xyz;
#endif
    vec3 lighting = diffuseColor.xyz * diffuseLight + getAmbientColor().xyz * ambientLight + emission;

    clampLightingResult(lighting);

    gl_FragData[0].xyz *= lighting;

    float shininess = gl_FrontMaterial.shininess;
    vec3 matSpec = getSpecularColor().xyz * specStrength;
#if @normalMap
    matSpec *= normalTex.a;
#endif

    if (matSpec != vec3(0.0))
        gl_FragData[0].xyz += getSpecular(viewNormal, normalize(passViewPos.xyz), shininess, matSpec) * shadowing;

    gl_FragData[0] = applyFogAtDist(gl_FragData[0], euclideanDepth, linearDepth, far);

#if defined(FORCE_OPAQUE) && FORCE_OPAQUE
    // having testing & blending isn't enough - we need to write an opaque pixel to be opaque
    gl_FragData[0].a = 1.0;
#endif

#if !defined(FORCE_OPAQUE) && !@disableNormals
    gl_FragData[1].xyz = viewNormal * 0.5 + 0.5;
#endif

    applyShadowDebugOverlay();
}
