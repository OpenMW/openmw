#version 120

#include "lib/core/vertex.h.glsl"

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
