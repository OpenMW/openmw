#version 330 compatibility
// Note: compatibility profile required to access gl_ModelViewMatrix 

#extension GL_OVR_multiview : require
#extension GL_OVR_multiview2 : require

layout(num_views = @numViews) in;

#include "lib/core/vertex.h.glsl"

uniform mat4 projectionMatrixMultiView[@numViews];

vec4 modelToClip(vec4 pos)
{
    return viewToClip(modelToView(pos));
}

vec4 modelToView(vec4 pos)
{
    return gl_ModelViewMatrix * pos;
}

vec4 viewToClip(vec4 pos)
{
    return projectionMatrixMultiView[gl_ViewID_OVR] * pos;
}
