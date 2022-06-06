uniform float near;
uniform sampler2D opaqueDepthTex;
uniform float particleSize;

float viewDepth(float depth)
{
#if @reverseZ
    depth = 1.0 - depth;
#endif
    return (near * far) / ((far - near) * depth - far);
}

float calcSoftParticleFade()
{
    const float falloffMultiplier = 0.33;
    const float contrast = 1.30;

    vec2 screenCoords = gl_FragCoord.xy / screenRes;
    float sceneDepth = viewDepth(texture2D(opaqueDepthTex, screenCoords).x);
    float particleDepth = viewDepth(gl_FragCoord.z);
    float falloff = particleSize * falloffMultiplier;
    float delta = particleDepth - sceneDepth;

    if (delta < 0.0)
        discard;

    const float shift = 0.845;

    return shift * pow(clamp(delta/falloff, 0.0, 1.0), contrast);
}
