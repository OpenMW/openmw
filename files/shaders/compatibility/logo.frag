#version 120

uniform sampler2D logo;
uniform float osg_FrameTime;

varying vec2 logoUV;

// The time is counted from the window creation, which goes right before the VFS initialization. The logo stays hidden
// at first so that it does not flash when the VFS is ready quickly.
const float fadeInDelay = 0.5;
const float fadeInDuration = 0.5;

void main()
{
    gl_FragColor = texture2D(logo, logoUV);
    gl_FragColor.a *= clamp((osg_FrameTime - fadeInDelay) / fadeInDuration, 0.0, 1.0);
}
