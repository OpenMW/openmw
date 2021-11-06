#version 120

#include "skypasses.glsl"

uniform mat4 projectionMatrix;
uniform int pass;

varying vec4 passColor;
varying vec2 diffuseMapUV;

void main()
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);
    passColor = gl_Color;

    if (pass == PASS_CLOUDS)
        diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
    else
        diffuseMapUV = gl_MultiTexCoord0.xy;
}
