#version 120

uniform mat4 projectionMatrix;

varying vec2 diffuseMapUV;
varying float alphaPassthrough;

#include "vertexcolors.glsl"

void main()
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);

    if (colorMode == 2)
        alphaPassthrough = gl_Color.a;
    else
        alphaPassthrough = gl_FrontMaterial.diffuse.a;

    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
