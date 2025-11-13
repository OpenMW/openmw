#version 400 core

#if @terrainDeformTess

layout(vertices = 3) out;

// Uniforms
uniform vec3 eyePos;                        // Camera position (from OpenMW)
uniform sampler2D terrainDeformationMap;    // Displacement texture
uniform vec2 deformationOffset;             // Player movement offset
uniform float deformationScale;             // World units per texel
uniform float baseTessLevel;                // Base tessellation level (4-16)
uniform float maxTessLevel;                 // Max tessellation level (32-64)
uniform int materialType;                   // Current terrain material type

// Input from vertex shader
in vec3 worldPos_TC_in[];
in vec2 uv_TC_in[];
in vec3 passNormal_TC_in[];
in vec3 passViewPos_TC_in[];

// Output to TES
out vec3 worldPos_TE_in[];
out vec2 uv_TE_in[];
out vec3 passNormal_TE_in[];
out vec3 passViewPos_TE_in[];

float getTessellationLevel(vec3 pos)
{
    // Factor 1: Distance-based LOD
    float dist = distance(eyePos, pos);
    float distanceFactor = clamp(1.0 - (dist / 2000.0), 0.0, 1.0); // 0-2000 units

    // Factor 2: Deformation amount (tessellate more near footprints)
    vec2 deformUV = (pos.xy + deformationOffset) / deformationScale;
    float deformation = texture(terrainDeformationMap, deformUV).r;
    float deformFactor = smoothstep(0.01, 0.1, deformation);

    // Factor 3: Material type (snow gets more detail)
    float materialFactor = 1.0;
    if (materialType == 1) materialFactor = 1.5;      // Snow
    else if (materialType == 2) materialFactor = 1.2; // Sand
    else if (materialType == 3) materialFactor = 1.0; // Ash

    // Combine factors
    float finalLevel = mix(baseTessLevel, maxTessLevel,
                          distanceFactor * 0.6 + deformFactor * 0.4);
    finalLevel *= materialFactor;

    return clamp(finalLevel, 1.0, maxTessLevel);
}

void main()
{
    // Pass through attributes
    worldPos_TE_in[gl_InvocationID] = worldPos_TC_in[gl_InvocationID];
    uv_TE_in[gl_InvocationID] = uv_TC_in[gl_InvocationID];
    passNormal_TE_in[gl_InvocationID] = passNormal_TC_in[gl_InvocationID];
    passViewPos_TE_in[gl_InvocationID] = passViewPos_TC_in[gl_InvocationID];

    // Calculate tessellation levels for each edge
    if (gl_InvocationID == 0)
    {
        vec3 center = (worldPos_TC_in[0] + worldPos_TC_in[1] + worldPos_TC_in[2]) / 3.0;
        float tessLevel = getTessellationLevel(center);

        // Set outer tessellation levels (one per edge)
        gl_TessLevelOuter[0] = tessLevel;
        gl_TessLevelOuter[1] = tessLevel;
        gl_TessLevelOuter[2] = tessLevel;

        // Set inner tessellation level
        gl_TessLevelInner[0] = tessLevel;
    }
}

#endif
