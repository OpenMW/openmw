#version 120

varying vec2 uv;

#include "lib/core/fragment.h.glsl"

void main()
{
    gl_FragColor = samplerLastShader(uv);
}