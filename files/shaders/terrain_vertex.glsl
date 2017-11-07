#version 120

#define SHADOWS @shadows_enabled

varying vec2 uv;
varying float depth;

#define PER_PIXEL_LIGHTING (@normalMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
varying vec4 lighting;
varying vec3 shadowDiffuseLighting;
#else
varying vec4 passColor;
#endif
varying vec3 passViewPos;
varying vec3 passNormal;

#if SHADOWS
	@shadow_texture_unit_declarations
	@shadow_space_coordinate_declarations
#endif // SHADOWS

#include "lighting.glsl"

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    depth = gl_Position.z;

    vec4 viewPos = (gl_ModelViewMatrix * gl_Vertex);
    gl_ClipVertex = viewPos;

#if !PER_PIXEL_LIGHTING
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
    lighting = doLighting(viewPos.xyz, viewNormal, gl_Color, shadowDiffuseLighting);
#else
    passColor = gl_Color;
#endif
    passNormal = gl_Normal.xyz;
    passViewPos = viewPos.xyz;

    uv = gl_MultiTexCoord0.xy;

	#if SHADOWS
		// This matrix has the opposite handedness to the others used here, so multiplication must have the vector to the left. Alternatively it could be transposed after construction, but that's extra work for the GPU just to make the code look a tiny bit cleaner.
		mat4 eyePlaneMat;
		@shadow_space_coordinate_calculations
	#endif // SHADOWS
}
