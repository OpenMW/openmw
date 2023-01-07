#version 120

uniform mat4 projectionMatrix;
varying vec2 diffuseMapUV;

#include "vertexcolors.glsl"

void main()
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);
    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
    passColor = gl_Color;
}