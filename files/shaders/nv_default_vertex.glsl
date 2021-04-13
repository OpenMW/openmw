#version 120

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#if @diffuseMap
varying vec2 diffuseMapUV;
#endif

#if @emissiveMap
varying vec2 emissiveMapUV;
#endif

#if @normalMap
varying vec2 normalMapUV;
varying vec4 passTangent;
#endif

varying float euclideanDepth;
varying float linearDepth;

varying vec3 passViewPos;
varying vec3 passNormal;

#define PER_PIXEL_LIGHTING 1

#include "vertexcolors.glsl"
#include "shadows_vertex.glsl"
#include "lighting.glsl"

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = gl_Position.z;

#if @diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMapUV] * gl_MultiTexCoord@diffuseMapUV).xy;
#endif

#if @emissiveMap
    emissiveMapUV = (gl_TextureMatrix[@emissiveMapUV] * gl_MultiTexCoord@emissiveMapUV).xy;
#endif

#if @normalMap
    normalMapUV = (gl_TextureMatrix[@normalMapUV] * gl_MultiTexCoord@normalMapUV).xy;
    passTangent = gl_MultiTexCoord7.xyzw;
#endif

    passColor = gl_Color;
    passViewPos = viewPos.xyz;
    passNormal = gl_Normal.xyz;

#if @shadows_enabled
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
    setupShadowCoords(viewPos, viewNormal);
#endif
}
