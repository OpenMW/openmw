#version 120

varying vec2 uv;
varying float depth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
varying vec4 lighting;
#else
varying vec3 passViewPos;
varying vec3 passViewNormal;
varying vec4 passColor;
#endif

#include "lighting.glsl"

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    depth = gl_Position.z;

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);

#if !PER_PIXEL_LIGHTING
    lighting = doLighting(viewPos.xyz, viewNormal, gl_Color);
#else
    passViewPos = viewPos.xyz;
    passViewNormal = viewNormal;
    passColor = gl_Color;
#endif

    uv = gl_MultiTexCoord0.xy;
}
