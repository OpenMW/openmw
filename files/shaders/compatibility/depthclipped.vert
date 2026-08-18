#version 120

varying vec2 diffuseMapUV;
varying float alphaPassthrough;

uniform mat4 texMat0;

#include "lib/core/vertex.h.glsl"
#include "vertexcolors.glsl"

void main()
{
    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;

    if (colorMode == 2)
        alphaPassthrough = gl_Color.a;
    else
        alphaPassthrough = gl_FrontMaterial.diffuse.a;

    diffuseMapUV = (texMat0 * gl_MultiTexCoord0).xy;
}
