#version 400 core

#if @terrainDeformTess

layout(triangles, fractional_odd_spacing, ccw) in;

// Uniforms
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat3 osg_NormalMatrix;
uniform sampler2D terrainDeformationMap;
uniform vec2 deformationOffset;
uniform float deformationScale;
uniform int materialType;
uniform float maxDisplacementDepth;  // Maximum displacement in world units (default: 50.0)

// Input from TCS
in vec3 worldPos_TE_in[];
in vec2 uv_TE_in[];
in vec3 passNormal_TE_in[];
in vec3 passViewPos_TE_in[];

// Output to fragment shader
out vec3 worldPos_FS;
out vec2 uv_FS;
out vec3 passNormal_FS;
out vec3 passViewPos_FS;
out float deformation_FS;

#include "lib/terrain/deformation.glsl"

// Interpolate barycentric coordinates
vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2)
{
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

void main()
{
    // Interpolate vertex attributes
    worldPos_FS = interpolate3D(worldPos_TE_in[0], worldPos_TE_in[1], worldPos_TE_in[2]);
    uv_FS = interpolate2D(uv_TE_in[0], uv_TE_in[1], uv_TE_in[2]);
    vec3 baseNormal = interpolate3D(passNormal_TE_in[0], passNormal_TE_in[1], passNormal_TE_in[2]);
    passViewPos_FS = interpolate3D(passViewPos_TE_in[0], passViewPos_TE_in[1], passViewPos_TE_in[2]);

    // Sample deformation texture
    vec2 deformUV = (worldPos_FS.xy + deformationOffset) / deformationScale;
    float deformValue = texture(terrainDeformationMap, deformUV).r;
    deformation_FS = deformValue;

    // Apply material-specific depth multiplier
    float depthMultiplier = getDepthMultiplier(materialType);

    // Displace vertex downward (negative Z in world space)
    float displacement = deformValue * depthMultiplier * maxDisplacementDepth;
    vec3 displacedPos = worldPos_FS;
    displacedPos.z -= displacement;

    // Calculate new normal by sampling neighbors for gradient
    float texelSize = 1.0 / 1024.0;  // Match deformation map size
    float hL = texture(terrainDeformationMap, deformUV + vec2(-texelSize, 0.0)).r;
    float hR = texture(terrainDeformationMap, deformUV + vec2(texelSize, 0.0)).r;
    float hD = texture(terrainDeformationMap, deformUV + vec2(0.0, -texelSize)).r;
    float hU = texture(terrainDeformationMap, deformUV + vec2(0.0, texelSize)).r;

    // Compute gradient in world space
    vec3 gradient = vec3(
        (hL - hR) * depthMultiplier * maxDisplacementDepth,
        (hD - hU) * depthMultiplier * maxDisplacementDepth,
        2.0 * deformationScale * texelSize
    );

    // Blend with base normal based on deformation amount
    vec3 deformedNormal = normalize(baseNormal + gradient);
    passNormal_FS = mix(baseNormal, deformedNormal, smoothstep(0.0, 0.2, deformValue));

    // Update view position with displacement
    vec4 viewPos = osg_ModelViewMatrix * vec4(displacedPos, 1.0);
    passViewPos_FS = viewPos.xyz;

    // Transform to clip space
    gl_Position = osg_ModelViewProjectionMatrix * vec4(displacedPos, 1.0);
}

#endif
