#version 330 compatibility

#extension GL_OVR_multiview : require
#extension GL_OVR_multiview2 : require

layout(num_views = @numViews) in;

#include "openmw_vertex.h.glsl"

uniform mat4 projectionMatrixMultiView[@numViews];

vec4 mw_modelToClip(vec4 pos)
{
    return mw_viewToClip(mw_modelToView(pos));
}

vec4 mw_modelToView(vec4 pos)
{
    return gl_ModelViewMatrix * pos;
}

vec4 mw_viewToClip(vec4 pos)
{
    return projectionMatrixMultiView[gl_ViewID_OVR] * pos;
}