#version 120

uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;

varying float alphaPassthrough;

uniform bool useDiffuseMapForShadowAlpha;
uniform bool alphaTestShadows;

void main()
{
    gl_FragData[0].rgb = vec3(1.0);
    if (useDiffuseMapForShadowAlpha)
        gl_FragData[0].a = texture2D(diffuseMap, diffuseMapUV).a * alphaPassthrough;
    else
        gl_FragData[0].a = alphaPassthrough;

    // Prevent translucent things casting shadow (including the player using an invisibility effect). For now, rely on the deprecated FF test for non-blended stuff.
    if (alphaTestShadows && gl_FragData[0].a <= 0.5)
        discard;
}
