#version 120

uniform sampler2D imageIn;

#define MAX_POSITIONS 100
uniform vec3 positions[MAX_POSITIONS];
uniform int positionCount;

uniform float osg_SimulationTime;
uniform vec2 offset;

#include "lib/water/ripples.glsl"

void main()
{
    vec2 uv = (gl_FragCoord.xy + offset) / @rippleMapSize;

    vec4 color = texture2D(imageIn, uv);
    float wavesizeMultiplier = getTemporalWaveSizeMultiplier(osg_SimulationTime);
    for (int i = 0; i < positionCount; ++i)
    {
        float wavesize = wavesizeMultiplier * positions[i].z;
        float displace = clamp(0.2 * abs(length((positions[i].xy + offset) - gl_FragCoord.xy) / wavesize - 1.0) + 0.8, 0.0, 1.0);
        color.rg = mix(vec2(-1.0), color.rg, displace);
    }

    gl_FragColor = color;
}
