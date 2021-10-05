#version 120

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

uniform mat4 projectionMatrix;

#if @defined diffuseMap
varying vec2 diffuseMapUV;
#endif

#if @defined darkMap
varying vec2 darkMapUV;
#endif

#if @defined detailMap
varying vec2 detailMapUV;
#endif

#if @defined decalMap
varying vec2 decalMapUV;
#endif

#if @defined emissiveMap
varying vec2 emissiveMapUV;
#endif

#if @defined normalMap
varying vec2 normalMapUV;
varying vec4 passTangent;
#endif

#if @defined envMap
varying vec2 envMapUV;
#endif

#if @defined bumpMap
varying vec2 bumpMapUV;
#endif

#if @defined specularMap
varying vec2 specularMapUV;
#endif

varying float euclideanDepth;
varying float linearDepth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec3 passLighting;
centroid varying vec3 shadowDiffuseLighting;
uniform float emissiveMult;
#endif
varying vec3 passViewPos;
varying vec3 passNormal;

#include "vertexcolors.glsl"
#include "shadows_vertex.glsl"

#include "lighting.glsl"
#include "depth.glsl"

void main(void)
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);

    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

#if (@defined envMap || !PER_PIXEL_LIGHTING || @shadows_enabled)
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
#endif

#if @defined envMap
    vec3 viewVec = normalize(viewPos.xyz);
    vec3 r = reflect( viewVec, viewNormal );
    float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
    envMapUV = vec2(r.x/m + 0.5, r.y/m + 0.5);
#endif

#if @defined diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMap] * gl_MultiTexCoord@diffuseMap).xy;
#endif

#if @defined darkMap
    darkMapUV = (gl_TextureMatrix[@darkMap] * gl_MultiTexCoord@darkMap).xy;
#endif

#if @defined detailMap
    detailMapUV = (gl_TextureMatrix[@detailMap] * gl_MultiTexCoord@detailMap).xy;
#endif

#if @defined decalMap
    decalMapUV = (gl_TextureMatrix[@decalMap] * gl_MultiTexCoord@decalMap).xy;
#endif

#if @defined emissiveMap
    emissiveMapUV = (gl_TextureMatrix[@emissiveMap] * gl_MultiTexCoord@emissiveMap).xy;
#endif

#if @defined normalMap
    normalMapUV = (gl_TextureMatrix[@normalMap] * gl_MultiTexCoord@normalMap).xy;
    passTangent = gl_MultiTexCoord7.xyzw;
#endif

#if @defined bumpMap
    bumpMapUV = (gl_TextureMatrix[@bumpMap] * gl_MultiTexCoord@bumpMap).xy;
#endif

#if @defined specularMap
    specularMapUV = (gl_TextureMatrix[@specularMap] * gl_MultiTexCoord@specularMap).xy;
#endif

    passColor = gl_Color;
    passViewPos = viewPos.xyz;
    passNormal = gl_Normal.xyz;

#if !PER_PIXEL_LIGHTING
    vec3 diffuseLight, ambientLight;
    doLighting(viewPos.xyz, viewNormal, diffuseLight, ambientLight, shadowDiffuseLighting);
    vec3 emission = getEmissionColor().xyz * emissiveMult;
    passLighting = getDiffuseColor().xyz * diffuseLight + getAmbientColor().xyz * ambientLight + emission;
    clampLightingResult(passLighting);
    shadowDiffuseLighting *= getDiffuseColor().xyz;
#endif

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}
