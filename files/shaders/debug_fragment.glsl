#version 120

#include "vertexcolors.glsl"

void main()
{
    gl_FragData[0] = getDiffuseColor();
}
