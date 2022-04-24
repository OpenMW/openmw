#version 120

#include "openmw_vertex.h.glsl"

varying vec4  position;
varying float linearDepth;

#include "shadows_vertex.glsl"
#include "depth.glsl"

void main(void)
{
    gl_Position = mw_modelToClip(gl_Vertex);

    position = gl_Vertex;

    vec4 viewPos = mw_modelToView(gl_Vertex);
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

    setupShadowCoords(viewPos, normalize((gl_NormalMatrix * gl_Normal).xyz));
}
