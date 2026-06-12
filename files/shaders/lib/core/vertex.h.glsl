@link "lib/core/vertex.glsl" if !@useOVR_multiview
@link "lib/core/vertex_multiview.glsl" if @useOVR_multiview
@link "lib/core/lighting_vertex.glsl" if @lightingMethodClustered
@link "lib/core/lighting_vertex_legacy.glsl" if !@lightingMethodClustered

vec4 modelToClip(vec4 pos);
vec4 modelToView(vec4 pos);
vec4 viewToClip(vec4 pos);
vec2 clipToScreen(vec4 pos);
void doLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight, out vec3 shadowDiffuse, out vec3 shadowSpecular);
