#version 120

uniform vec2 scaling;

varying vec2 uv;

#include "lib/core/vertex.h.glsl"

void main()
{
    gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
    uv = (gl_Position.xy * 0.5 + 0.5) * scaling;
}
