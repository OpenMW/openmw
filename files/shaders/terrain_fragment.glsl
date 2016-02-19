#version 120

varying vec2 uv;

uniform sampler2D diffuseMap;

#if @normalMap
uniform sampler2D normalMap;
#endif

#if @blendMap
uniform sampler2D blendMap;
#endif

varying float depth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
varying vec4 lighting;
#else
varying vec3 passViewPos;
varying vec3 passViewNormal;
varying vec4 passColor;
#endif

#include "lighting.glsl"

void main()
{
    vec2 diffuseMapUV = (gl_TextureMatrix[0] * vec4(uv, 0.0, 1.0)).xy;

    gl_FragData[0] = vec4(texture2D(diffuseMap, diffuseMapUV).xyz, 1.0);

#if @blendMap
    vec2 blendMapUV = (gl_TextureMatrix[1] * vec4(uv, 0.0, 1.0)).xy;
    gl_FragData[0].a *= texture2D(blendMap, blendMapUV).a;
#endif

#if PER_PIXEL_LIGHTING
    vec3 viewNormal = passViewNormal;
#endif

#if @normalMap
    vec3 normalTex = texture2D(normalMap, diffuseMapUV).xyz;

    vec3 viewTangent = (gl_ModelViewMatrix * vec4(1.0, 0.0, 0.0, 0.0)).xyz;
    vec3 viewBinormal = normalize(cross(viewTangent, viewNormal));
    viewTangent = normalize(cross(viewNormal, viewBinormal)); // note, now we need to re-cross to derive tangent again because it wasn't orthonormal
    mat3 tbn = mat3(viewTangent, viewBinormal, viewNormal);

    viewNormal = normalize(tbn * (normalTex * 2.0 - 1.0));
#endif


#if !PER_PIXEL_LIGHTING
    gl_FragData[0] *= lighting;
#else
    gl_FragData[0] *= doLighting(passViewPos, normalize(viewNormal), passColor);
#endif

    float fogValue = clamp((depth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, gl_Fog.color.xyz, fogValue);
}
