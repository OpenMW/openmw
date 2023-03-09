#version 120

#include "lib/core/vertex.h.glsl"

varying vec4  position;
varying float linearDepth;

#include "shadows_vertex.glsl"
#include "lib/view/depth.glsl"

void main(void)
{
    gl_Position = modelToClip(gl_Vertex);

    position = gl_Vertex;

    vec4 viewPos = modelToView(gl_Vertex);
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

    setupShadowCoords(viewPos, normalize((gl_NormalMatrix * gl_Normal).xyz));
}
