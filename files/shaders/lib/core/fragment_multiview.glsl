#version 330

#extension GL_OVR_multiview : require
#extension GL_OVR_multiview2 : require
#extension GL_EXT_texture_array : require

#include "lib/core/fragment.h.glsl"

uniform sampler2DArray reflectionMap;

vec4 sampleReflectionMap(vec2 uv)
{
    return texture(reflectionMap, vec3((uv), gl_ViewID_OVR));
}

uniform sampler2DArray lastShader;

vec4 samplerLastShader(vec2 uv)
{
    return texture(lastShader, vec3((uv), gl_ViewID_OVR));
}

#if @skyBlending
uniform sampler2DArray sky;

vec3 sampleSkyColor(vec2 uv)
{
    return texture(sky, vec3((uv), gl_ViewID_OVR)).xyz;
}
#endif

uniform sampler2DArray opaqueDepthTex;
uniform sampler2DArray opaqueColorTex;

vec4 sampleOpaqueDepthTex(vec2 uv)
{
    return texture(opaqueDepthTex, vec3((uv), gl_ViewID_OVR));
}

vec4 sampleOpaqueColorTex(vec2 uv)
{
    return texture(opaqueColorTex, vec3((uv), gl_ViewID_OVR));
}
