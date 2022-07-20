#ifndef OPENMW_FRAGMENT_H_GLSL
#define OPENMW_FRAGMENT_H_GLSL

@link "openmw_fragment.glsl" if !@useOVR_multiview
@link "openmw_fragment_multiview.glsl" if @useOVR_multiview

vec4 mw_sampleReflectionMap(vec2 uv);

#if @refraction_enabled
vec4 mw_sampleRefractionMap(vec2 uv);
float mw_sampleRefractionDepthMap(vec2 uv);
#endif

vec4 mw_samplerLastShader(vec2 uv);

#if @skyBlending
vec3 mw_sampleSkyColor(vec2 uv);
#endif

#endif  // OPENMW_FRAGMENT_H_GLSL
