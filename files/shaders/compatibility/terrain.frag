#version 120

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

varying vec2 uv;

uniform sampler2D diffuseMap;

#if @normalMap
uniform sampler2D normalMap;
#endif

#if @blendMap
uniform sampler2D blendMap;
#endif

varying float euclideanDepth;
varying float linearDepth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec3 passLighting;
centroid varying vec3 shadowDiffuseLighting;
#endif
varying vec3 passViewPos;
varying vec3 passNormal;

uniform vec2 screenRes;
uniform float far;

#include "vertexcolors.glsl"
#include "shadows_fragment.glsl"
#include "lib/light/lighting.glsl"
#include "lib/material/parallax.glsl"
#include "fog.glsl"

void main()
{
    vec2 adjustedUV = (gl_TextureMatrix[0] * vec4(uv, 0.0, 1.0)).xy;

    vec3 normal = normalize(passNormal);

#if @normalMap
    vec4 normalTex = texture2D(normalMap, adjustedUV);

    vec3 normalizedNormal = normal;
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = normalize(cross(tangent, normalizedNormal));
    tangent = normalize(cross(normalizedNormal, binormal)); // note, now we need to re-cross to derive tangent again because it wasn't orthonormal
    mat3 tbnTranspose = mat3(tangent, binormal, normalizedNormal);

    normal = tbnTranspose * (normalTex.xyz * 2.0 - 1.0);
#endif

#if @parallax
    vec3 cameraPos = (gl_ModelViewMatrixInverse * vec4(0,0,0,1)).xyz;
    vec3 objectPos = (gl_ModelViewMatrixInverse * vec4(passViewPos, 1)).xyz;
    vec3 eyeDir = normalize(cameraPos - objectPos);
    adjustedUV += getParallaxOffset(eyeDir, tbnTranspose, normalTex.a, 1.f);

    // update normal using new coordinates
    normalTex = texture2D(normalMap, adjustedUV);

    normal = tbnTranspose * (normalTex.xyz * 2.0 - 1.0);
#endif

    vec3 viewNormal = normalize(gl_NormalMatrix * normal);

    vec4 diffuseTex = texture2D(diffuseMap, adjustedUV);
    gl_FragData[0] = vec4(diffuseTex.xyz, 1.0);

#if @blendMap
    vec2 blendMapUV = (gl_TextureMatrix[1] * vec4(uv, 0.0, 1.0)).xy;
    gl_FragData[0].a *= texture2D(blendMap, blendMapUV).a;
#endif

    vec4 diffuseColor = getDiffuseColor();
    gl_FragData[0].a *= diffuseColor.a;

    float shadowing = unshadowedLightRatio(linearDepth);
    vec3 lighting;
#if !PER_PIXEL_LIGHTING
    lighting = passLighting + shadowDiffuseLighting * shadowing;
#else
    vec3 diffuseLight, ambientLight;
    doLighting(passViewPos, viewNormal, shadowing, diffuseLight, ambientLight);
    lighting = diffuseColor.xyz * diffuseLight + getAmbientColor().xyz * ambientLight + getEmissionColor().xyz;
#endif

    clampLightingResult(lighting);

    gl_FragData[0].xyz *= lighting;

#if @specularMap
    float shininess = 128.0; // TODO: make configurable
    vec3 matSpec = vec3(diffuseTex.a);
#else
    float shininess = gl_FrontMaterial.shininess;
    vec3 matSpec = getSpecularColor().xyz;
#endif

    if (matSpec != vec3(0.0))
    {
        gl_FragData[0].xyz += getSpecular(viewNormal, normalize(passViewPos), shininess, matSpec) * shadowing;
    }

    gl_FragData[0] = applyFogAtDist(gl_FragData[0], euclideanDepth, linearDepth, far);

#if !@disableNormals && @writeNormals
    gl_FragData[1].xyz = viewNormal * 0.5 + 0.5;
#endif

    applyShadowDebugOverlay();
}
