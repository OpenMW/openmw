#version 120

#include "lib/core/fragment.h.glsl"

uniform sampler2D reflectionMap;

vec4 sampleReflectionMap(vec2 uv)
{
    return texture2D(reflectionMap, uv);
}

uniform sampler2D lastShader;

vec4 samplerLastShader(vec2 uv)
{
    return texture2D(lastShader, uv);
}

#if @skyBlending
uniform sampler2D sky;

vec3 sampleSkyColor(vec2 uv)
{
    return texture2D(sky, uv).xyz;
}
#endif

uniform sampler2D opaqueDepthTex;
uniform sampler2D opaqueColorTex;

vec4 sampleOpaqueDepthTex(vec2 uv)
{
    return texture2D(opaqueDepthTex, uv);
}

vec4 sampleOpaqueColorTex(vec2 uv)
{
    return texture2D(opaqueColorTex, uv);
}
