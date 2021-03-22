#version 120

#if @diffuseMap
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

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;
#if @radialFog
    euclideanDepth = length(viewPos.xyz);
#else
    linearDepth = gl_Position.z;
#endif

#if @diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMapUV] * gl_MultiTexCoord@diffuseMapUV).xy;
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
