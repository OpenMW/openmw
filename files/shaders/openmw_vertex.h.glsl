@link "openmw_vertex.glsl"

vec4 mw_modelToClip(vec4 pos);
vec4 mw_modelToView(vec4 pos);
vec4 mw_viewToClip(vec4 pos);
vec4 mw_viewStereoAdjust(vec4 pos);
mat4 mw_viewMatrix();
mat4 mw_projectionMatrix();