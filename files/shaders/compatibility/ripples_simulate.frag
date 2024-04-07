#version 120

uniform sampler2D imageIn;

#include "lib/water/ripples.glsl"

void main()
{
    vec2 uv = gl_FragCoord.xy / @rippleMapSize;

    float pixelSize = 1.0 / @rippleMapSize;

    float oneOffset = pixelSize;
    float oneAndHalfOffset = 1.5 * pixelSize;

    vec4 n = vec4(
        texture2D(imageIn, uv + vec2(oneOffset, 0.0)).r,
        texture2D(imageIn, uv + vec2(-oneOffset, 0.0)).r,
        texture2D(imageIn, uv + vec2(0.0, oneOffset)).r,
        texture2D(imageIn, uv + vec2(0.0, -oneOffset)).r
    );

    vec4 n2 = vec4(
        texture2D(imageIn, uv + vec2(oneAndHalfOffset, 0.0)).r,
        texture2D(imageIn, uv + vec2(-oneAndHalfOffset, 0.0)).r,
        texture2D(imageIn, uv + vec2(0.0, oneAndHalfOffset)).r,
        texture2D(imageIn, uv + vec2(0.0, -oneAndHalfOffset)).r
    );

    vec4 color = texture2D(imageIn, uv);

    gl_FragColor = applySprings(color, n, n2);
}
