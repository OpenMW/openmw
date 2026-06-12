#version 120

#include "lib/core/vertex.h.glsl"

uniform vec2 screenRes;
uniform mat4 projectionMatrix;

vec4 modelToClip(vec4 pos)
{
    return projectionMatrix * modelToView(pos);
}

vec4 modelToView(vec4 pos)
{
    return gl_ModelViewMatrix * pos;
}

vec4 viewToClip(vec4 pos)
{
    return projectionMatrix * pos;
}

vec2 clipToScreen(vec4 pos)
{
    return ((pos.xy / pos.w) * 0.5 + 0.5) * screenRes;
}
