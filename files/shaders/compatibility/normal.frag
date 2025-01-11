#version 120
#pragma import_defines(TERRAIN, OBJECT, DdiffuseMap, DnormalMap, DblendMap, Dparallax)

uniform sampler2D diffuseMap;

#if defined(DnormalMap) && DnormalMap
    uniform sampler2D normalMap;
#endif

#if defined(DblendMap) && DblendMap
    uniform sampler2D blendMap;
#endif

varying float alphaPassthrough;

varying vec3 passNormal;
varying vec3 passViewPos;

#include "lib/material/parallax.glsl"
#include "compatibility/normals.glsl"
#include "vertexcolors.glsl"
varying vec2 uv;

void main()
{
    vec2 adjustedUV = (gl_TextureMatrix[0] * vec4(uv, 0.0, 1.0)).xy;

#if defined(Dparallax) && Dparallax
    adjustedUV += getParallaxOffset(transpose(normalToViewMatrix) * normalize(-passViewPos), texture2D(normalMap, adjustedUV).a, 1.f);
#endif

    gl_FragData[0].a = 1.0;

#if defined(OBJECT) && OBJECT
    vec4 diffuseTex = texture2D(diffuseMap, adjustedUV);
    gl_FragData[0] = vec4(diffuseTex.xyz, 1.0);

    float alpha = diffuseTex.a * alphaPassthrough;
    const float alphaRef = 0.499;

  //  vec4 diffuseColor = getDiffuseColor();
 //   gl_FragData[0].a *= diffuseColor.a;

    if (alpha < alphaRef)
        discard;

 //   gl_FragData[0] = vec4(diffuseTex.xyz, 1.0);


#endif

#if defined(DblendMap) && DblendMap
    vec2 blendMapUV = (gl_TextureMatrix[1] * vec4(uv, 0.0, 1.0)).xy;
    gl_FragData[0].a *= texture2D(blendMap, blendMapUV).a;
#endif

#if defined(DnormalMap) && DnormalMap
    vec4 normalTex = texture2D(normalMap, adjustedUV);
    vec3 normal = normalTex.xyz * 2.0 - 1.0;
    vec3 viewNormal = normalToView(normal);
#else
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
#endif

    gl_FragData[0].rgb = viewNormal * 0.5 + 0.5;
/*
#if defined(TERRAIN) && TERRAIN
    gl_FragData[0].rgb *= gl_FragData[0].a;
#endif
*/

}
