#version 120

varying vec2 uv;
varying float euclideanDepth;
varying float linearDepth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec4 lighting;
centroid varying vec3 shadowDiffuseLighting;
#endif
centroid varying vec4 passColor;
varying vec3 passViewPos;
varying vec3 passNormal;

#include "shadows_vertex.glsl"

#include "lighting.glsl"

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = gl_Position.z;

#if (!PER_PIXEL_LIGHTING || @shadows_enabled)
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
#endif

#if !PER_PIXEL_LIGHTING
    lighting = doLighting(viewPos.xyz, viewNormal, gl_Color, shadowDiffuseLighting);
#endif
    passColor = gl_Color;
    passNormal = gl_Normal.xyz;
    passViewPos = viewPos.xyz;

    uv = gl_MultiTexCoord0.xy;

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}
