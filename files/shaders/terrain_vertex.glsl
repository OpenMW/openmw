#version 120

varying vec2 uv;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !@accurateFog
varying float depth;
#endif

#if !PER_PIXEL_LIGHTING
centroid varying vec4 lighting;
#else
centroid varying vec4 passColor;
#endif
varying vec3 passViewPos;
varying vec3 passNormal;

#include "lighting.glsl"

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
#if !@accurateFog
    depth = gl_Position.z;
#endif

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;

#if !PER_PIXEL_LIGHTING
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
    lighting = doLighting(viewPos.xyz, viewNormal, gl_Color);
#else
    passColor = gl_Color;
#endif
    passNormal = gl_Normal.xyz;
    passViewPos = viewPos.xyz;

    uv = gl_MultiTexCoord0.xy;
}
