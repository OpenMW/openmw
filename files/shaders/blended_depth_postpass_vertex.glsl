#version 120

uniform mat4 projectionMatrix;

varying vec2 diffuseMapUV;
varying float alphaPassthrough;

#include "openmw_vertex.h.glsl"
#include "vertexcolors.glsl"

void main()
{
    gl_Position = mw_modelToClip(gl_Vertex);

    if (colorMode == 2)
        alphaPassthrough = gl_Color.a;
    else
        alphaPassthrough = gl_FrontMaterial.diffuse.a;

    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
