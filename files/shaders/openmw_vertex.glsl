#version 120

#include "openmw_vertex.h.glsl"

uniform mat4 projectionMatrix;

vec4 mw_modelToClip(vec4 pos)
{
    return projectionMatrix * mw_modelToView(pos);
}

vec4 mw_modelToView(vec4 pos)
{
    return gl_ModelViewMatrix * pos;
}

vec4 mw_viewToClip(vec4 pos)
{
    return projectionMatrix * pos;
}

vec4 mw_viewStereoAdjust(vec4 pos)
{
    return pos;
}

mat4 mw_viewMatrix()
{
    return gl_ModelViewMatrix;
}

mat4 mw_projectionMatrix()
{
    return projectionMatrix;
}
