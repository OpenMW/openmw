#version 120

uniform sampler2D imageIn;
uniform int positionCount;
uniform vec4 positions[100]; // xyz = position, w = materialType
uniform int materialTypes[100];

#include "lib/terrain/deformation.glsl"

void main()
{
    vec2 uv = gl_FragCoord.xy / @deformMapSize;
    vec2 pixelPos = gl_FragCoord.xy;

    // Read current deformation value
    float currentDeform = texture2D(imageIn, uv).r;
    float newDeform = currentDeform;

    // Apply all footprint stamps
    for (int i = 0; i < positionCount && i < 100; ++i)
    {
        vec3 stampPos = positions[i].xyz;
        int materialType = materialTypes[i];

        // stampPos.z contains the footprint size in cell units
        float footprintRadius = stampPos.z;

        // Calculate distance from current pixel to stamp center
        vec2 stampCenter = stampPos.xy;
        float dist = distance(pixelPos, stampCenter);

        // Only apply if within radius
        if (dist < footprintRadius)
        {
            float falloff = getFootprintFalloff(dist, footprintRadius);
            float depthMultiplier = getDepthMultiplier(materialType);

            // Footprint depth (0.0 to 1.0, where 1.0 = maximum displacement)
            float stampDepth = falloff * depthMultiplier * 0.5; // 0.5 = max depth factor

            // Add to existing deformation (accumulate multiple footprints)
            newDeform = min(newDeform + stampDepth, 1.0);
        }
    }

    gl_FragColor = vec4(newDeform, 0.0, 0.0, 1.0);
}
