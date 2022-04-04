#version @GLSLVersion

#include "vertexcolors.glsl"

void main()
{
    gl_FragData[0] = getDiffuseColor();
}
