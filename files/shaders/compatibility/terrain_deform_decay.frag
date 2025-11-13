#version 120

uniform sampler2D imageIn;

void main()
{
    vec2 uv = gl_FragCoord.xy / @deformMapSize;

    // Read current deformation value
    float currentDeform = texture2D(imageIn, uv).r;

    // Apply decay - gradually fade deformation over time
    // Using a simple exponential decay
    float decayRate = 0.995; // Decay by 0.5% per frame
    float newDeform = currentDeform * decayRate;

    // Snap to zero when very small to avoid floating point issues
    if (newDeform < 0.001)
        newDeform = 0.0;

    gl_FragColor = vec4(newDeform, 0.0, 0.0, 1.0);
}
