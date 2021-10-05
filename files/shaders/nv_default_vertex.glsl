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

#if @defined emissiveMap
varying vec2 emissiveMapUV;
#endif

#if @defined normalMap
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
#include "depth.glsl"

void main(void)
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

#if @defined diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMap] * gl_MultiTexCoord@diffuseMap).xy;
#endif

#if @defined emissiveMap
    emissiveMapUV = (gl_TextureMatrix[@emissiveMap] * gl_MultiTexCoord@emissiveMap).xy;
#endif

#if @defined normalMap
    normalMapUV = (gl_TextureMatrix[@normalMap] * gl_MultiTexCoord@normalMap).xy;
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
