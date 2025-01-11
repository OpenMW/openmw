#version 120

varying vec2 diffuseMapUV;
varying float alphaPassthrough;
varying vec3 passNormal;

#include "lib/core/vertex.h.glsl"
#include "vertexcolors.glsl"

void main()
{
    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;

    passNormal = gl_Normal.xyz;

    if (colorMode == 2)
        alphaPassthrough = gl_Color.a;
    else
        alphaPassthrough = gl_FrontMaterial.diffuse.a;

    diffuseMapUV = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
