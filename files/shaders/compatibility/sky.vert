#version 120

#include "lib/core/vertex.h.glsl"

#include "lib/sky/passes.glsl"

uniform int pass;
uniform mat4 texMat0;

varying vec4 passColor;
varying vec2 diffuseMapUV;

void main()
{
    gl_Position = modelToClip(gl_Vertex);
    passColor = gl_Color;

    if (pass == PASS_CLOUDS)
        diffuseMapUV = (texMat0 * vec4(gl_MultiTexCoord0.xyz, 1.0)).xy;
    else
        diffuseMapUV = gl_MultiTexCoord0.xy;
}
