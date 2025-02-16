#ifndef OPENMW_FRAGMENT_H_GLSL
#define OPENMW_FRAGMENT_H_GLSL

uniform sampler2D reflectionMap;

vec4 sampleReflectionMap(vec2 uv)
{
    return texture2D(reflectionMap, uv);
}

#if @waterRefraction
uniform sampler2D refractionMap;
uniform highp sampler2D refractionDepthMap;

vec4 sampleRefractionMap(vec2 uv)
{
    return texture2D(refractionMap, uv);
}

float sampleRefractionDepthMap(vec2 uv)
{
    return texture2D(refractionDepthMap, uv).x;
}

#endif

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

vec4 sampleOpaqueDepthTex(vec2 uv)
{
    return texture2D(opaqueDepthTex, uv);
}

#endif  // OPENMW_FRAGMENT_H_GLSL
