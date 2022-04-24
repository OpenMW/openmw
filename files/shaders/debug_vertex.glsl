#version 120

#include "openmw_vertex.h.glsl"

centroid varying vec4 passColor;

void main()
{
    gl_Position = mw_modelToClip(gl_Vertex);

    passColor = gl_Color;
}
