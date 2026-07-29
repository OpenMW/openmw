#version 120

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#define PER_PIXEL_LIGHTING 1

#include "lib/core/vertex.h.glsl"

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

uniform mat4 texMat@diffuseMapUV;
uniform mat4 texMat@emissiveMapUV;
uniform mat4 texMat@normalMapUV;

#include "lib/view/depth.glsl"

#include "compatibility/vertexcolors.glsl"
#include "compatibility/shadows_vertex.glsl"
#include "compatibility/normals.glsl"

void main(void)
{
    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);
    passColor = gl_Color;
    passViewPos = viewPos.xyz;
    passNormal = gl_Normal.xyz;
    normalToViewMatrix = gl_NormalMatrix;

#if @normalMap
    normalToViewMatrix *= generateTangentSpace(gl_MultiTexCoord7.xyzw, passNormal);
#endif

#if @diffuseMap
    diffuseMapUV = (texMat@diffuseMapUV * vec4(gl_MultiTexCoord@diffuseMapUV.xyz, 1.0)).xy;
#endif

#if @emissiveMap
    emissiveMapUV = (texMat@emissiveMapUV * vec4(gl_MultiTexCoord@emissiveMapUV.xyz, 1.0)).xy;
#endif

#if @normalMap
    normalMapUV = (texMat@normalMapUV * vec4(gl_MultiTexCoord@normalMapUV.xyz, 1.0)).xy;
#endif


#if @shadows_enabled
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
    setupShadowCoords(viewPos, viewNormal);
#endif
}
