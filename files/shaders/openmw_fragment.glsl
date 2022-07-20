#version 120

#include "openmw_fragment.h.glsl"

uniform sampler2D reflectionMap;

vec4 mw_sampleReflectionMap(vec2 uv)
{
    return texture2D(reflectionMap, uv);
}

#if @refraction_enabled
uniform sampler2D refractionMap;
uniform sampler2D refractionDepthMap;

vec4 mw_sampleRefractionMap(vec2 uv)
{
    return texture2D(refractionMap, uv);
}

float mw_sampleRefractionDepthMap(vec2 uv)
{
    return texture2D(refractionDepthMap, uv).x;
}

#endif

uniform sampler2D omw_SamplerLastShader;

vec4 mw_samplerLastShader(vec2 uv)
{
    return texture2D(omw_SamplerLastShader, uv);
}

#if @skyBlending
uniform sampler2D sky;

vec3 mw_sampleSkyColor(vec2 uv)
{
    return texture2D(sky, uv).xyz;
}
#endif
