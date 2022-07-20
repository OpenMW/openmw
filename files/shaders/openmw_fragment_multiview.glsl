#version 330 compatibility

#extension GL_OVR_multiview : require
#extension GL_OVR_multiview2 : require
#extension GL_EXT_texture_array : require

#include "openmw_fragment.h.glsl"

uniform sampler2DArray reflectionMap;

vec4 mw_sampleReflectionMap(vec2 uv)
{
    return texture2DArray(reflectionMap, vec3((uv), gl_ViewID_OVR));
}

#if @refraction_enabled

uniform sampler2DArray refractionMap;
uniform sampler2DArray refractionDepthMap;

vec4 mw_sampleRefractionMap(vec2 uv)
{
    return texture2DArray(refractionMap, vec3((uv), gl_ViewID_OVR));
}

float mw_sampleRefractionDepthMap(vec2 uv)
{
    return texture2DArray(refractionDepthMap, vec3((uv), gl_ViewID_OVR)).x;
}

#endif

uniform sampler2DArray omw_SamplerLastShader;

vec4 mw_samplerLastShader(vec2 uv)
{
    return texture2DArray(omw_SamplerLastShader, vec3((uv), gl_ViewID_OVR));
}

#if @skyBlending
uniform sampler2DArray sky;

vec3 mw_sampleSkyColor(vec2 uv)
{
    return texture2DArray(sky, vec3((uv), gl_ViewID_OVR)).xyz;
}
#endif
