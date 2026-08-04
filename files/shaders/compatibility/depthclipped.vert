#version 120

varying vec2 diffuseMapUV;
varying float alphaPassthrough;

#include "lib/core/vertex.h.glsl"
#include "lib/material/vertexcolors.glsl"

void main()
{
    Material material = getMaterial();

    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;

    if (material.colorMode == 2)
        alphaPassthrough = gl_Color.a;
    else
        alphaPassthrough = material.diffuse.a;

    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
