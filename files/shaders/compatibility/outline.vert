#version 120

#include "lib/core/vertex.h.glsl"

void main(void)
{
    gl_Position = modelToClip(gl_Vertex);
    gl_ClipVertex = modelToView(gl_Vertex);
}