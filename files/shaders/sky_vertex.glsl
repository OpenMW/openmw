#version 120

#include "openmw_vertex.h.glsl"

#include "skypasses.glsl"

uniform int pass;

varying vec4 passColor;
varying vec2 diffuseMapUV;

void main()
{
    gl_Position = mw_modelToClip(gl_Vertex);
    passColor = gl_Color;

    if (pass == PASS_CLOUDS)
        diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
    else
        diffuseMapUV = gl_MultiTexCoord0.xy;
}
