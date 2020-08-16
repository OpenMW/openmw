#version 120

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

#define PER_PIXEL_LIGHTING @forcePPL

varying float euclideanDepth;
varying float linearDepth;

#if !@forcePPL
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

#if @forcePPL
    vec3 viewNormal = gl_NormalMatrix * normalize(passNormal);
#endif

#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, adjustedDiffuseUV);
#else
    gl_FragData[0] = vec4(1.0);
#endif

    float shadowing = unshadowedLightRatio(linearDepth);
    if (euclideanDepth > @grassFadeStart)
        gl_FragData[0].a *= 1.0-smoothstep(@grassFadeStart, @grassFadeEnd, euclideanDepth);

#if !@forcePPL

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
#if !@forcePPL
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
