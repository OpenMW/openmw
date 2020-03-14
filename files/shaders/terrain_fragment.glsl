#version 120

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
centroid varying vec4 lighting;
centroid varying vec3 shadowDiffuseLighting;
#else
centroid varying vec4 passColor;
#endif
varying vec3 passViewPos;
varying vec3 passNormal;

#include "shadows_fragment.glsl"
#include "lighting.glsl"
#include "parallax.glsl"

void main()
{
    vec2 adjustedUV = (gl_TextureMatrix[0] * vec4(uv, 0.0, 1.0)).xy;

#if @normalMap
    vec4 normalTex = texture2D(normalMap, adjustedUV);

    vec3 normalizedNormal = normalize(passNormal);
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = normalize(cross(tangent, normalizedNormal));
    tangent = normalize(cross(normalizedNormal, binormal)); // note, now we need to re-cross to derive tangent again because it wasn't orthonormal
    mat3 tbnTranspose = mat3(tangent, binormal, normalizedNormal);

    vec3 viewNormal = normalize(gl_NormalMatrix * (tbnTranspose * (normalTex.xyz * 2.0 - 1.0)));
#else
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
#endif

#if @parallax
    vec3 cameraPos = (gl_ModelViewMatrixInverse * vec4(0,0,0,1)).xyz;
    vec3 objectPos = (gl_ModelViewMatrixInverse * vec4(passViewPos, 1)).xyz;
    vec3 eyeDir = normalize(cameraPos - objectPos);
    adjustedUV += getParallaxOffset(eyeDir, tbnTranspose, normalTex.a, 1.f);

    // update normal using new coordinates
    normalTex = texture2D(normalMap, adjustedUV);
    viewNormal = normalize(gl_NormalMatrix * (tbnTranspose * (normalTex.xyz * 2.0 - 1.0)));
#endif

    vec4 diffuseTex = texture2D(diffuseMap, adjustedUV);
    gl_FragData[0] = vec4(diffuseTex.xyz, 1.0);

#if @blendMap
    vec2 blendMapUV = (gl_TextureMatrix[1] * vec4(uv, 0.0, 1.0)).xy;
    gl_FragData[0].a *= texture2D(blendMap, blendMapUV).a;
#endif

    float shadowing = unshadowedLightRatio(linearDepth);

#if !PER_PIXEL_LIGHTING

#if @clamp
    gl_FragData[0] *= clamp(lighting + vec4(shadowDiffuseLighting * shadowing, 0), vec4(0.0), vec4(1.0));
#else
    gl_FragData[0] *= lighting + vec4(shadowDiffuseLighting * shadowing, 0);
#endif

#else
    gl_FragData[0] *= doLighting(passViewPos, normalize(viewNormal), passColor, shadowing);
#endif

#if @specularMap
    float shininess = 128; // TODO: make configurable
    vec3 matSpec = vec3(diffuseTex.a, diffuseTex.a, diffuseTex.a);
#else
    float shininess = gl_FrontMaterial.shininess;
    vec3 matSpec = gl_FrontMaterial.specular.xyz;
#endif

    gl_FragData[0].xyz += getSpecular(normalize(viewNormal), normalize(passViewPos), shininess, matSpec) * shadowing;

#if @radialFog
    float fogValue = clamp((euclideanDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#else
    float fogValue = clamp((linearDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#endif
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, gl_Fog.color.xyz, fogValue);

    applyShadowDebugOverlay();
}
