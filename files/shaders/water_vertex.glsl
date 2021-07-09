#version 120

uniform mat4 projectionMatrix;

varying vec3  screenCoordsPassthrough;
varying vec4  position;
varying float linearDepth;

#include "shadows_vertex.glsl"
#include "depth.glsl"

void main(void)
{
    gl_Position = projectionMatrix * (gl_ModelViewMatrix * gl_Vertex);

    mat4 scalemat = mat4(0.5, 0.0, 0.0, 0.0,
                         0.0, -0.5, 0.0, 0.0,
                         0.0, 0.0, 0.5, 0.0,
                         0.5, 0.5, 0.5, 1.0);

    vec4 texcoordProj = ((scalemat) * ( gl_Position));
    screenCoordsPassthrough = texcoordProj.xyw;

    position = gl_Vertex;

    vec4 viewPos = gl_ModelViewMatrix * gl_Vertex;
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

    setupShadowCoords(viewPos, normalize((gl_NormalMatrix * gl_Normal).xyz));
}
