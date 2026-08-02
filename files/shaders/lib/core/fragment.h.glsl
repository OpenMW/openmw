#ifndef OPENMW_FRAGMENT_H_GLSL
#define OPENMW_FRAGMENT_H_GLSL

@link "lib/core/fragment.glsl" if !@useOVR_multiview
@link "lib/core/fragment_multiview.glsl" if @useOVR_multiview
@link "lib/core/lighting_fragment.glsl" if @lightingMethodClustered
@link "lib/core/lighting_fragment_legacy.glsl" if !@lightingMethodClustered

vec4 sampleReflectionMap(vec2 uv);

#if @waterRefraction
vec4 sampleRefractionMap(vec2 uv);
float sampleRefractionDepthMap(vec2 uv);
#endif

vec4 samplerLastShader(vec2 uv);

#if @skyBlending
vec3 sampleSkyColor(vec2 uv);
#endif

vec4 sampleOpaqueDepthTex(vec2 uv);

void doLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal, float shininess, float shadowing, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight);

vec3 doSpecularLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal);

#endif  // OPENMW_FRAGMENT_H_GLSL
