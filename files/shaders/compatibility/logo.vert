#version 120

uniform float osg_FrameTime;
uniform float aspectRatio;
uniform vec2 turnCenter;
uniform float turnSpeed;

varying vec2 logoUV;

// The star is upright halfway through the fade-in, see fadeInDelay and fadeInDuration in logo.frag.
const float uprightTime = 0.75;

void main()
{
    float angle = turnSpeed * (osg_FrameTime - uprightTime);
    mat2 rotation = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
    vec2 position = turnCenter + rotation * (gl_Vertex.xy - turnCenter);
    // Squeezes the logo along the longer side of the window to keep it square.
    if (aspectRatio > 1.0)
        position.x /= aspectRatio;
    else
        position.y *= aspectRatio;
    gl_Position = vec4(position, gl_Vertex.zw);
    logoUV = gl_MultiTexCoord0.xy;
}
