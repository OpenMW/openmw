#version 120

uniform mat4 projectionMatrix;

#if @defined diffuseMap
varying vec2 diffuseMapUV;
#endif

#if @radialFog
varying float euclideanDepth;
#else
varying float linearDepth;
#endif

uniform bool useFalloff;
uniform vec4 falloffParams;

varying vec3 passViewPos;
varying float passFalloff;

#include "vertexcolors.glsl"
#include "depth.glsl"

void main(void)
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;
#if @radialFog
    euclideanDepth = length(viewPos.xyz);
#else
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);
#endif

#if @defined diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMap] * gl_MultiTexCoord@diffuseMap).xy;
#endif

    passColor = gl_Color;
    if (useFalloff)
    {
        vec3 viewNormal = gl_NormalMatrix * normalize(gl_Normal.xyz);
        vec3 viewDir = normalize(viewPos.xyz);
        float viewAngle = abs(dot(viewNormal, viewDir));
        passFalloff = smoothstep(falloffParams.y, falloffParams.x, viewAngle);
    }
    else
    {
        passFalloff = 1.0;
    }
}
