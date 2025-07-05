#version 120

uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;

varying float alphaPassthrough;

uniform bool useDiffuseMapForShadowAlpha;
uniform bool alphaTestShadows;
uniform float alphaRef;

#include "lib/material/alpha.glsl"

void main()
{
    gl_FragData[0].rgb = vec3(1.0);
    if (useDiffuseMapForShadowAlpha)
        gl_FragData[0].a = texture2D(diffuseMap, diffuseMapUV).a * alphaPassthrough;
    else
        gl_FragData[0].a = alphaPassthrough;

    gl_FragData[0].a = alphaTest(gl_FragData[0].a, alphaRef);

    // Prevent translucent things casting shadow (including the player using an invisibility effect).
    // This replaces alpha blending, which obviously doesn't work with depth buffers
    if (alphaTestShadows && gl_FragData[0].a <= 0.5)
        discard;
}
