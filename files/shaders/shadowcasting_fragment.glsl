varying float alphaPassthrough;

uniform bool useDiffuseMapForShadowAlpha;
uniform bool alphaTestShadows;

void main()
{
    gl_FragData[0].rgb = vec3(1.0);
#if @diffuseMap
    if (useDiffuseMapForShadowAlpha)
        gl_FragData[0].a = texture2D(diffuseMap, diffuseMapUV).a * alphaPassthrough;
    else
#endif
        gl_FragData[0].a = alphaPassthrough;

    alphaTest();

    // Prevent translucent things casting shadow (including the player using an invisibility effect).
    // This replaces alpha blending, which obviously doesn't work with depth buffers
    if (alphaTestShadows && gl_FragData[0].a <= 0.5)
        discard;
}
