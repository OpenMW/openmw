#version 120

varying vec2 uv;
uniform sampler2D luminanceSceneTex;
uniform sampler2D prevLuminanceSceneTex;

uniform float osg_DeltaFrameTime;

void main()
{
    float prevLum = texture2D(prevLuminanceSceneTex, vec2(0.5, 0.5)).r;
    float currLum = texture2D(luminanceSceneTex, vec2(0.5, 0.5)).r;

    float avgLum = exp2((currLum * @logLumRange) + @minLog);
    gl_FragColor.r = prevLum + (avgLum - prevLum) * (1.0 - exp(-osg_DeltaFrameTime * @hdrExposureTime));
}
