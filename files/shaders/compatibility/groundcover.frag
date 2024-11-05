#version 120
#pragma import_defines(CLASSIC_FALLOFF, MAX_LIGHTS)

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#define GROUNDCOVER

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

#if @normalMap
uniform sampler2D normalMap;
varying vec2 normalMapUV;
#endif

// Other shaders respect forcePPL, but legacy groundcover mods were designed to work with vertex lighting.
// They may do not look as intended with per-pixel lighting, so ignore this setting for now.
#define PER_PIXEL_LIGHTING @normalMap

varying float euclideanDepth;
varying float linearDepth;
uniform vec2 screenRes;
uniform float far;
uniform float alphaRef;

#if PER_PIXEL_LIGHTING
varying vec3 passViewPos;
#else
centroid varying vec3 passLighting;
centroid varying vec3 shadowDiffuseLighting;
#endif

varying vec3 passNormal;

#include "shadows_fragment.glsl"
#include "lib/light/lighting.glsl"
#include "lib/material/alpha.glsl"
#include "fog.glsl"
#include "compatibility/normals.glsl"

void main()
{
#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV);
#else
    gl_FragData[0] = vec4(1.0);
#endif

    if (euclideanDepth > @groundcoverFadeStart)
        gl_FragData[0].a *= 1.0-smoothstep(@groundcoverFadeStart, @groundcoverFadeEnd, euclideanDepth);

    gl_FragData[0].a = alphaTest(gl_FragData[0].a, alphaRef);

#if @normalMap
    vec4 normalTex = texture2D(normalMap, normalMapUV);
    vec3 normal = normalTex.xyz * 2.0 - 1.0;
#if @reconstructNormalZ
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
#endif
    vec3 viewNormal = normalToView(normal);
#else
    vec3 viewNormal = normalToView(normalize(passNormal));
#endif

    float shadowing = unshadowedLightRatio(linearDepth);

    vec3 lighting;
#if !PER_PIXEL_LIGHTING
    lighting = passLighting + shadowDiffuseLighting * shadowing;
#else
    vec3 diffuseLight, ambientLight, specularLight;
    doLighting(passViewPos, viewNormal, gl_FrontMaterial.shininess, shadowing, diffuseLight, ambientLight, specularLight);
    lighting = diffuseLight + ambientLight;
#endif

    clampLightingResult(lighting);

    gl_FragData[0].xyz *= lighting;
    gl_FragData[0] = applyFogAtDist(gl_FragData[0], euclideanDepth, linearDepth, far);

#if !@disableNormals
    gl_FragData[1].xyz = viewNormal * 0.5 + 0.5;
#endif

    applyShadowDebugOverlay();
}
