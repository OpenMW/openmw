#version 330

#extension GL_OVR_multiview : require
#extension GL_OVR_multiview2 : require

#include "lib/core/fragment.h.glsl"

uniform sampler2DArray reflectionMap;

vec4 sampleReflectionMap(vec2 uv)
{
    return texture(reflectionMap, vec3((uv), gl_ViewID_OVR));
}

#if @waterRefraction

uniform sampler2DArray refractionMap;
uniform sampler2DArray refractionDepthMap;

vec4 sampleRefractionMap(vec2 uv)
{
    return texture(refractionMap, vec3((uv), gl_ViewID_OVR));
}

float sampleRefractionDepthMap(vec2 uv)
{
    return texture(refractionDepthMap, vec3((uv), gl_ViewID_OVR)).x;
}

#endif

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
