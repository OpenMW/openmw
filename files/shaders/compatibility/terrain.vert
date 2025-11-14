#version 120

#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#include "lib/core/vertex.h.glsl"
varying vec2 uv;
varying float euclideanDepth;
varying float linearDepth;

#define PER_PIXEL_LIGHTING (@normalMap || @specularMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec3 passLighting;
centroid varying vec3 passSpecular;
centroid varying vec3 shadowDiffuseLighting;
centroid varying vec3 shadowSpecularLighting;
#endif
varying vec3 passViewPos;
varying vec3 passNormal;

#include "vertexcolors.glsl"
#include "shadows_vertex.glsl"
#include "compatibility/normals.glsl"

#include "lib/light/lighting.glsl"
#include "lib/view/depth.glsl"

#if @terrainDeformation
#include "lib/terrain/deformation.glsl"

uniform sampler2D terrainDeformationMap;
uniform vec2 deformationOffset;
uniform float deformationScale;
uniform int materialType;
uniform float maxDisplacementDepth;
uniform mat4 osg_ViewMatrixInverse;
#endif

void main(void)
{
    vec4 modelPos = gl_Vertex;
    vec3 modelNormal = gl_Normal.xyz;

#if @terrainDeformation
    // Convert model position to world space for deformation texture sampling
    vec4 worldPos4 = osg_ModelMatrix * modelPos;
    vec3 worldPos = worldPos4.xyz;

    // Sample deformation texture
    vec2 deformUV = (worldPos.xy + deformationOffset) / deformationScale;
    float deformValue = texture2D(terrainDeformationMap, deformUV).r;

    // Apply material-specific depth multiplier
    float depthMultiplier = getDepthMultiplier(materialType);

    // Displace vertex downward in model space (negative Z)
    float displacement = clamp(deformValue, 0.0, 1.0) * depthMultiplier * maxDisplacementDepth;
    modelPos.z -= displacement;

    // Calculate new normal by sampling neighbors for gradient
    float texelSize = 1.0 / 1024.0;  // Match deformation map size (1024x1024)
    float hL = texture2D(terrainDeformationMap, deformUV + vec2(-texelSize, 0.0)).r;
    float hR = texture2D(terrainDeformationMap, deformUV + vec2(texelSize, 0.0)).r;
    float hD = texture2D(terrainDeformationMap, deformUV + vec2(0.0, -texelSize)).r;
    float hU = texture2D(terrainDeformationMap, deformUV + vec2(0.0, texelSize)).r;

    // Compute gradient in world space (how much terrain slopes in X and Y)
    vec3 gradient = vec3(
        (hL - hR) * depthMultiplier * maxDisplacementDepth,
        (hD - hU) * depthMultiplier * maxDisplacementDepth,
        2.0 * deformationScale * texelSize
    );

    // Blend with base normal based on deformation amount
    vec3 deformedNormal = normalize(modelNormal + gradient);
    modelNormal = mix(modelNormal, deformedNormal, smoothstep(0.0, 0.2, deformValue));
#endif

    gl_Position = modelToClip(modelPos);

    vec4 viewPos = modelToView(modelPos);
    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

    passColor = gl_Color;
    passNormal = modelNormal;
    passViewPos = viewPos.xyz;
    normalToViewMatrix = gl_NormalMatrix;

#if @normalMap
    mat3 tbnMatrix = generateTangentSpace(vec4(1.0, 0.0, 0.0, -1.0), passNormal);
    tbnMatrix[0] = -normalize(cross(tbnMatrix[2], tbnMatrix[1]));
    normalToViewMatrix *= tbnMatrix;
#endif

#if !PER_PIXEL_LIGHTING || @shadows_enabled
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
#endif

#if !PER_PIXEL_LIGHTING
    vec3 diffuseLight, ambientLight, specularLight;
    doLighting(viewPos.xyz, viewNormal, gl_FrontMaterial.shininess, diffuseLight, ambientLight, specularLight, shadowDiffuseLighting, shadowSpecularLighting);
    passLighting = getDiffuseColor().xyz * diffuseLight + getAmbientColor().xyz * ambientLight + getEmissionColor().xyz;
    passSpecular = getSpecularColor().xyz * specularLight;
    clampLightingResult(passLighting);
    shadowDiffuseLighting *= getDiffuseColor().xyz;
    shadowSpecularLighting *= getSpecularColor().xyz;
#endif

    uv = gl_MultiTexCoord0.xy;

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}
