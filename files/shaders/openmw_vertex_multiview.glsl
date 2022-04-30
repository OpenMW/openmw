#version 330 compatibility

#extension GL_OVR_multiview : require
#extension GL_OVR_multiview2 : require

layout(num_views = @numViews) in;

#include "openmw_vertex.h.glsl"

uniform mat4 projectionMatrixMultiView[@numViews];
uniform mat4 viewMatrixMultiView[@numViews];

vec4 mw_modelToClip(vec4 pos)
{
    return projectionMatrixMultiView[gl_ViewID_OVR] * mw_modelToView(pos);
}

vec4 mw_modelToView(vec4 pos)
{
    return viewMatrixMultiView[gl_ViewID_OVR] * gl_ModelViewMatrix * pos;
}

vec4 mw_viewToClip(vec4 pos)
{
    return projectionMatrixMultiView[gl_ViewID_OVR] * viewMatrixMultiView[gl_ViewID_OVR] * pos;
}

vec4 mw_viewStereoAdjust(vec4 pos)
{
    return viewMatrixMultiView[gl_ViewID_OVR] * pos;
}
