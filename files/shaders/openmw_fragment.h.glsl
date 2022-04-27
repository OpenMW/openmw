@link "openmw_fragment.glsl"

vec4 mw_sampleReflectionMap(vec2 uv);

#if @refraction_enabled
vec4 mw_sampleRefractionMap(vec2 uv);
float mw_sampleRefractionDepthMap(vec2 uv);
#endif