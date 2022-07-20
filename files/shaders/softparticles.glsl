uniform float near;
uniform sampler2D opaqueDepthTex;
uniform float particleSize;
uniform bool particleFade;

float viewDepth(float depth)
{
#if @reverseZ
    depth = 1.0 - depth;
#endif
    return (near * far) / ((far - near) * depth - far);
}

float calcSoftParticleFade(in vec3 viewDir, in vec3 viewNormal, in vec3 viewPos)
{
    float euclidianDepth = length(viewPos);

    const float falloffMultiplier = 0.33;
    const float contrast = 1.30;

    vec2 screenCoords = gl_FragCoord.xy / screenRes;

    float depth = texture2D(opaqueDepthTex, screenCoords).x;

    float sceneDepth = viewDepth(depth);
    float particleDepth = passViewPos.z;
    float falloff = particleSize * falloffMultiplier;
    float delta = particleDepth - sceneDepth;

    const float nearMult = 300.0;
    float viewBias = 1.0;

    if (particleFade) {
        float VdotN = dot(viewDir, viewNormal);
        viewBias = abs(VdotN) * quickstep(euclidianDepth / nearMult) * (1.0 - pow(1.0 + VdotN, 1.3));
    }

    const float shift = 0.845;
    return shift * pow(clamp(delta/falloff, 0.0, 1.0), contrast) * viewBias;
}
