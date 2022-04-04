#ifndef MULTIVIEW_FRAGMENT
#define MULTIVIEW_FRAGMENT

// This file either enables or disables GL_OVR_multiview2 related code.
// For use in fragment shaders

// REQUIREMENT:
// GLSL version: 330 or greater
// GLSL profile: compatibility
// NOTE: If stereo is enabled using Misc::StereoView::shaderStereoDefines, version 330 compatibility (or greater) will be set.
//
// This file provides symbols for sampling stereo-aware textures. Without multiview, these texture uniforms are sampler2D,
// while in stereo the same uniforms are sampler2DArray instead. The symbols defined in this file mask this difference, allowing
// the same code to work in both cases. Use mw_stereoAwareSampler2D and mw_stereoAwareTexture2D, where you otherwise would use 
// sampler2D and texture2D()
//
// USAGE:
// For stereo-aware textures, such as reflections, use the mw_stereoAwareSampler2D sampler and mw_stereoAwareTexture2D method 
// instead of the usual sampler2D and texture2D.
//
// Using water reflection as an example, the old code for these textures changes from
//    uniform sampler2D reflectionMap;
//    ...
//    vec3 reflection = texture2D(reflectionMap, screenCoords + screenCoordsOffset).rgb;
//    
// to
//    uniform mw_stereoAwareSampler2D reflectionMap;
//    ...
//    vec3 reflection = mw_stereoAwareTexture2D(reflectionMap, screenCoords + screenCoordsOffset).rgb;
//

#if @useOVR_multiview

#extension GL_OVR_multiview : require
#extension GL_OVR_multiview2 : require
#extension GL_EXT_texture_array : require

#define mw_stereoAwareSampler2D sampler2DArray
#define mw_stereoAwareTexture2D(texture, uv) texture2DArray(texture, vec3((uv), gl_ViewID_OVR))

#else // useOVR_multiview

#define mw_stereoAwareSampler2D sampler2D
#define mw_stereoAwareTexture2D(texture, uv) texture2D(texture, uv)

#endif // useOVR_multiview

#endif // MULTIVIEW_FRAGMENT