#version 120

varying vec2 uv;
uniform sampler2D luminanceSceneTex;
uniform sampler2D prevLuminanceSceneTex;

void main()
{
    float prevLum = texture2D(prevLuminanceSceneTex, vec2(0.5, 0.5)).r;

    float l = texture2D(luminanceSceneTex, vec2(0.5, 0.5)).r;
    float weightedAvgLum = exp2((l * @logLumRange) + @minLog);
    gl_FragColor.r = prevLum + (weightedAvgLum - prevLum) * @hdrExposureTime;
}
