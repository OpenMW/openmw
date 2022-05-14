#version 120

varying vec2 uv;

#include "openmw_fragment.h.glsl"

void main()
{
    gl_FragColor = mw_samplerLastShader(uv);
}