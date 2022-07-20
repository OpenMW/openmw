#version 120

varying vec2 uv;

#include "openmw_vertex.h.glsl"

void main()
{
    gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
    uv = gl_Position.xy * 0.5 + 0.5;
}
