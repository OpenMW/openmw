#ifndef MULTIVIEW_VERTEX
#define MULTIVIEW_VERTEX

// This file either enables or disables GL_OVR_multiview related code.
// For use in vertex shaders

// REQUIREMENT:
// GLSL version: 330 or greater
// GLSL profile: compatibility
// NOTE: If stereo is enabled using Misc::StereoView::shaderStereoDefines, version 330 compatibility (or greater) will be set.

// USAGE:
// To create a stereo-aware vertex shader, use the matrix accessor functions defined in this .glsl file to compute gl_Position.
// For the vertex stage, usually only gl_Position needs to be computed with stereo awareness, while other variables such as viewPos 
// should be computed in the center camera's view space and take no stereo awareness.
//
// A typical gl_Position line will look like the following:
//    gl_Position = mw_stereoAwareProjectionMatrix() * (mw_stereoAwareModelViewMatrix() * gl_Vertex);
//
// If you need to perform intermediate computations before determining the final values of gl_Position and viewPos,
// your code might look more like the following:
//    vec4 intermediateViewPos = gl_ModelViewMatrix * gl_Vertex;
//    vec4 viewPos = myWhateverCode(intermediateViewPos);
//    gl_Position = mw_stereoAwareProjectionMatrix() * mw_stereoAwareViewPosition(viewPos);
//

#if @useOVR_multiview

#extension GL_OVR_multiview : require

#ifndef MULTIVIEW_FRAGMENT
// Layout cannot be used in the fragment shader
layout(num_views = @numViews) in;
#endif

uniform mat4 projectionMatrixMultiView[@numViews];
uniform mat4 viewMatrixMultiView[@numViews];

// NOTE:
// stereo-aware inverse view matrices and normal matrices have not been implemented.
// Some effects like specular highlights would need stereo aware normal matrices to be 100% correct.
// But the difference is not likely to be noticeable unless you're actively looking for it.

mat4 mw_stereoAwareProjectionMatrix()
{
    return projectionMatrixMultiView[gl_ViewID_OVR];
}

mat4 mw_stereoAwareModelViewMatrix()
{
    return viewMatrixMultiView[gl_ViewID_OVR] * gl_ModelViewMatrix;
}

vec4 mw_stereoAwareViewPosition(vec4 viewPos)
{
	return viewMatrixMultiView[gl_ViewID_OVR] * viewPos;
}

#else // useOVR_multiview

uniform mat4 projectionMatrix;

mat4 mw_stereoAwareProjectionMatrix()
{
    return projectionMatrix;
}

mat4 mw_stereoAwareModelViewMatrix()
{
    return gl_ModelViewMatrix;
}

vec4 mw_stereoAwareViewPosition(vec4 viewPos)
{
	return viewPos;
}

#endif // useOVR_multiview

#endif // MULTIVIEW_VERTEX