@link "openmw_vertex.glsl" if !@useOVR_multiview
@link "openmw_vertex_multiview.glsl" if @useOVR_multiview

vec4 mw_modelToClip(vec4 pos);
vec4 mw_modelToView(vec4 pos);
vec4 mw_viewToClip(vec4 pos);
vec4 mw_viewStereoAdjust(vec4 pos);