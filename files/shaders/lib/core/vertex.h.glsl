@link "lib/core/vertex.glsl" if !@useOVR_multiview
@link "lib/core/vertex_multiview.glsl" if @useOVR_multiview
@link "lib/core/lighting_vertex.glsl" if @lightingMethodClustered
@link "lib/core/lighting_vertex_legacy.glsl" if !@lightingMethodClustered

#include "lib/material/struct.glsl"

vec4 modelToClip(vec4 pos);
vec4 modelToView(vec4 pos);
vec4 viewToClip(vec4 pos);
vec2 clipToScreen(vec4 pos);
void directionalLighting(vec3 viewDir, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight);
void pointLighting(vec2 screenCoord, vec3 viewDir, vec3 viewPos, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight);
Material getMaterial();
