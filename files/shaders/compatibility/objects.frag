#version 120
#pragma import_defines(FORCE_OPAQUE)

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

#if @darkMap
uniform sampler2D darkMap;
varying vec2 darkMapUV;
#endif

#if @detailMap
uniform sampler2D detailMap;
varying vec2 detailMapUV;
#endif

#if @decalMap
uniform sampler2D decalMap;
varying vec2 decalMapUV;
#endif

#if @emissiveMap
uniform sampler2D emissiveMap;
varying vec2 emissiveMapUV;
#endif

#if @normalMap
uniform sampler2D normalMap;
varying vec2 normalMapUV;
#endif

#if @envMap
uniform sampler2D envMap;
varying vec2 envMapUV;
uniform vec4 envMapColor;
#endif

#if @specularMap
uniform sampler2D specularMap;
varying vec2 specularMapUV;
#endif

#if @bumpMap
uniform sampler2D bumpMap;
varying vec2 bumpMapUV;
uniform vec2 envMapLumaBias;
uniform mat2 bumpMapMatrix;
#endif

#if @glossMap
uniform sampler2D glossMap;
varying vec2 glossMapUV;
#endif

uniform vec2 screenRes;
uniform float near;
uniform float far;
uniform float alphaRef;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec3 passLighting;
centroid varying vec3 shadowDiffuseLighting;
#else
uniform float emissiveMult;
#endif
uniform float specStrength;
varying vec3 passViewPos;
varying vec3 passNormal;
#if @normalMap || @diffuseParallax
varying vec4 passTangent;
#endif

#if @additiveBlending
#define ADDITIVE_BLENDING
#endif

#include "lib/light/lighting.glsl"
#include "lib/material/parallax.glsl"
#include "lib/material/alpha.glsl"

#include "fog.glsl"
#include "vertexcolors.glsl"
#include "shadows_fragment.glsl"
#include "compatibility/normals.glsl"

#if @softParticles
#include "lib/particle/soft.glsl"

uniform sampler2D opaqueDepthTex;
uniform float particleSize;
uniform bool particleFade;
uniform float softFalloffDepth;
#endif

#if @particleOcclusion
#include "lib/particle/occlusion.glsl"
uniform sampler2D orthoDepthMap;
varying vec3 orthoDepthMapCoord;
#endif

void main()
{
#if @particleOcclusion
    applyOcclusionDiscard(orthoDepthMapCoord, texture2D(orthoDepthMap, orthoDepthMapCoord.xy * 0.5 + 0.5).r);
#endif

    // only offset diffuse and normal maps for now, other textures are more likely to be using a completely different UV set
    vec2 offset = vec2(0.0);

#if @parallax || @diffuseParallax
#if @parallax
    float height = texture2D(normalMap, normalMapUV).a;
    float flipY = (passTangent.w > 0.0) ? -1.f : 1.f;
#else
    float height = texture2D(diffuseMap, diffuseMapUV).a;
    // FIXME: shouldn't be necessary, but in this path false-positives are common
    float flipY = -1.f;
#endif
    offset = getParallaxOffset(transpose(normalToViewMatrix) * normalize(-passViewPos), height, flipY);
#endif

#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV + offset);
#if @diffuseParallax
    gl_FragData[0].a = 1.0;
#else
    gl_FragData[0].a *= coveragePreservingAlphaScale(diffuseMap, diffuseMapUV + offset);
#endif
#else
    gl_FragData[0] = vec4(1.0);
#endif

    vec4 diffuseColor = getDiffuseColor();
    gl_FragData[0].a *= diffuseColor.a;

#if @darkMap
    gl_FragData[0] *= texture2D(darkMap, darkMapUV);
    gl_FragData[0].a *= coveragePreservingAlphaScale(darkMap, darkMapUV);
#endif

    gl_FragData[0].a = alphaTest(gl_FragData[0].a, alphaRef);

#if @normalMap
    vec3 viewNormal = normalToView(texture2D(normalMap, normalMapUV + offset).xyz * 2.0 - 1.0);
#else
    vec3 viewNormal = normalToView(normalize(passNormal));
#endif

    vec3 viewVec = normalize(passViewPos);

#if @detailMap
    gl_FragData[0].xyz *= texture2D(detailMap, detailMapUV).xyz * 2.0;
#endif

#if @decalMap
    vec4 decalTex = texture2D(decalMap, decalMapUV);
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, decalTex.xyz, decalTex.a * diffuseColor.a);
#endif

#if @envMap

    vec2 envTexCoordGen = envMapUV;
    float envLuma = 1.0;

#if @normalMap
    // if using normal map + env map, take advantage of per-pixel normals for envTexCoordGen
    vec3 r = reflect( viewVec, viewNormal );
    float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
    envTexCoordGen = vec2(r.x/m + 0.5, r.y/m + 0.5);
#endif

#if @bumpMap
    vec4 bumpTex = texture2D(bumpMap, bumpMapUV);
    envTexCoordGen += bumpTex.rg * bumpMapMatrix;
    envLuma = clamp(bumpTex.b * envMapLumaBias.x + envMapLumaBias.y, 0.0, 1.0);
#endif

    vec3 envEffect = texture2D(envMap, envTexCoordGen).xyz * envMapColor.xyz * envLuma;

#if @glossMap
    envEffect *= texture2D(glossMap, glossMapUV).xyz;
#endif

#if @preLightEnv
    gl_FragData[0].xyz += envEffect;
#endif

#endif

    float shadowing = unshadowedLightRatio(-passViewPos.z);
    vec3 lighting;
#if !PER_PIXEL_LIGHTING
    lighting = passLighting + shadowDiffuseLighting * shadowing;
#else
    vec3 diffuseLight, ambientLight;
    doLighting(passViewPos, viewNormal, shadowing, diffuseLight, ambientLight);
    vec3 emission = getEmissionColor().xyz * emissiveMult;
    lighting = diffuseColor.xyz * diffuseLight + getAmbientColor().xyz * ambientLight + emission;
#endif

    clampLightingResult(lighting);

    gl_FragData[0].xyz *= lighting;

#if @envMap && !@preLightEnv
    gl_FragData[0].xyz += envEffect;
#endif

#if @emissiveMap
    gl_FragData[0].xyz += texture2D(emissiveMap, emissiveMapUV).xyz;
#endif

#if @specularMap
    vec4 specTex = texture2D(specularMap, specularMapUV);
    float shininess = specTex.a * 255.0;
    vec3 matSpec = specTex.xyz;
#else
    float shininess = gl_FrontMaterial.shininess;
    vec3 matSpec = getSpecularColor().xyz;
#endif

    matSpec *= specStrength;
    if (matSpec != vec3(0.0))
    {
        gl_FragData[0].xyz += matSpec * getSpecular(viewNormal, passViewPos, shininess, shadowing);
    }

    gl_FragData[0] = applyFogAtPos(gl_FragData[0], passViewPos, far);

    vec2 screenCoords = gl_FragCoord.xy / screenRes;
#if !defined(FORCE_OPAQUE) && @softParticles
    gl_FragData[0].a *= calcSoftParticleFade(
        viewVec,
        passViewPos,
        viewNormal,
        near,
        far,
        texture2D(opaqueDepthTex, screenCoords).x,
        particleSize,
        particleFade,
        softFalloffDepth
    );
#endif

#if defined(FORCE_OPAQUE) && FORCE_OPAQUE
    // having testing & blending isn't enough - we need to write an opaque pixel to be opaque
    gl_FragData[0].a = 1.0;
#endif

#if !defined(FORCE_OPAQUE) && !@disableNormals
    gl_FragData[1].xyz = viewNormal * 0.5 + 0.5;
#endif

    applyShadowDebugOverlay();
}
