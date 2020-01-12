#version 120

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

#if @normalMap
uniform sampler2D normalMap;
varying vec2 normalMapUV;
varying vec4 passTangent;
#endif

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

varying float euclideanDepth;
varying float linearDepth;

#if !PER_PIXEL_LIGHTING
centroid varying vec4 lighting;
centroid varying vec3 shadowDiffuseLighting;
#endif
centroid varying vec4 passColor;
varying vec3 passViewPos;
varying vec3 passNormal;

#include "shadows_fragment.glsl"
#include "lighting.glsl"

void main()
{
#if @diffuseMap
    vec2 adjustedDiffuseUV = diffuseMapUV;
#endif

#if @normalMap
    vec4 normalTex = texture2D(normalMap, normalMapUV);

    vec3 normalizedNormal = normalize(passNormal);
    vec3 normalizedTangent = normalize(passTangent.xyz);
    vec3 binormal = cross(normalizedTangent, normalizedNormal) * passTangent.w;
    mat3 tbnTranspose = mat3(normalizedTangent, binormal, normalizedNormal);

    vec3 viewNormal = gl_NormalMatrix * normalize(tbnTranspose * (normalTex.xyz * 2.0 - 1.0));
#endif

#if (!@normalMap && @forcePPL)
    vec3 viewNormal = gl_NormalMatrix * normalize(passNormal);
#endif

#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, adjustedDiffuseUV);
#else
    gl_FragData[0] = vec4(1.0);
#endif

    float shadowing = unshadowedLightRatio(linearDepth);
    if (euclideanDepth > @groundcoverFadeStart)
        gl_FragData[0].a *= 1.0-smoothstep(@groundcoverFadeStart, @groundcoverFadeEnd, euclideanDepth);

#if !PER_PIXEL_LIGHTING

#if @clamp
    gl_FragData[0] *= clamp(lighting + vec4(shadowDiffuseLighting * shadowing, 0), vec4(0.0), vec4(1.0));
#else
    gl_FragData[0] *= lighting + vec4(shadowDiffuseLighting * shadowing, 0);
#endif

#else
    if(gl_FragData[0].a != 0.0)
        gl_FragData[0] *= doLighting(passViewPos, normalize(viewNormal), passColor, shadowing, true);
#endif

    float shininess = gl_FrontMaterial.shininess;
    vec3 matSpec;
    if (colorMode == ColorMode_Specular)
        matSpec = passColor.xyz;
    else
        matSpec = gl_FrontMaterial.specular.xyz;

    if (matSpec != vec3(0.0))
    {
#if (!@normalMap && !@forcePPL)
        vec3 viewNormal = gl_NormalMatrix * normalize(passNormal);
#endif
        gl_FragData[0].xyz += getSpecular(normalize(viewNormal), normalize(passViewPos.xyz), shininess, matSpec) * shadowing;
    }
#if @radialFog
    float fogValue = clamp((euclideanDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#else
    float fogValue = clamp((linearDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#endif
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, gl_Fog.color.xyz, fogValue);

    applyShadowDebugOverlay();
}
