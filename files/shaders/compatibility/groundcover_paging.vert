#version 120
#pragma import_defines(CLASSIC_FALLOFF, MAX_LIGHTS)
/*
#if @useUBO
    #extension GL_ARB_uniform_buffer_object : require
#endif

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif
*/
#include "lib/core/vertex.h.glsl"

#define GROUNDCOVER

attribute float originalHeight;

#if @diffuseMap
varying vec2 diffuseMapUV;
#endif

#if @normalMap
varying vec2 normalMapUV;
#endif

// Other shaders respect forcePPL, but legacy groundcover mods were designed to work with vertex lighting.
// They may do not look as intended with per-pixel lighting, so ignore this setting for now.
#define PER_PIXEL_LIGHTING @normalMap

varying float euclideanDepth;
varying float linearDepth;

#if PER_PIXEL_LIGHTING
varying vec3 passViewPos;
#else
centroid varying vec3 passLighting;
centroid varying vec3 shadowDiffuseLighting;
#endif

varying vec3 passNormal;

#include "shadows_vertex.glsl"
#include "compatibility/normals.glsl"
#include "lib/light/lighting.glsl"
#include "lib/view/depth.glsl"

uniform float osg_SimulationTime;
uniform mat4 osg_ViewMatrixInverse;
uniform mat4 osg_ViewMatrix;
uniform float windSpeed;
uniform vec3 playerPos;

#if @groundcoverStompMode == 0
#else
    #define STOMP 1
    #if @groundcoverStompMode == 2
        #define STOMP_HEIGHT_SENSITIVE 1
    #endif
    #define STOMP_INTENSITY_LEVEL @groundcoverStompIntensity
#endif

vec2 groundcoverDisplacement(in vec3 worldpos, float h)
{
    vec2 windDirection = vec2(1.0);
    vec3 footPos = playerPos;
    vec3 windVec = vec3(windSpeed * windDirection, 1.0);

    float v = length(windVec);
    vec2 displace = vec2(2.0 * windVec + 0.1);
    vec2 harmonics = vec2(0.0);

    harmonics += vec2((1.0 - 0.10*v) * sin(1.0*osg_SimulationTime + worldpos.xy / 1100.0));
    harmonics += vec2((1.0 - 0.04*v) * cos(2.0*osg_SimulationTime + worldpos.xy / 750.0));
    harmonics += vec2((1.0 + 0.14*v) * sin(3.0*osg_SimulationTime + worldpos.xy / 500.0));
    harmonics += vec2((1.0 + 0.28*v) * sin(5.0*osg_SimulationTime + worldpos.xy / 200.0));

    vec2 stomp = vec2(0.0);
#if STOMP
    float d = length(worldpos.xy - footPos.xy);
#if STOMP_INTENSITY_LEVEL == 0
    // Gentle intensity
    const float STOMP_RANGE = 50.0; // maximum distance from player that grass is affected by stomping
    const float STOMP_DISTANCE = 20.0; // maximum distance stomping can move grass
#elif STOMP_INTENSITY_LEVEL == 1
    // Reduced intensity
    const float STOMP_RANGE = 80.0;
    const float STOMP_DISTANCE = 40.0;
#elif STOMP_INTENSITY_LEVEL == 2
    // MGE XE intensity
    const float STOMP_RANGE = 150.0;
    const float STOMP_DISTANCE = 60.0;
#endif
    if (d < STOMP_RANGE && d > 0.0)
        stomp = (STOMP_DISTANCE / d - STOMP_DISTANCE / STOMP_RANGE) * (worldpos.xy - footPos.xy);

#ifdef STOMP_HEIGHT_SENSITIVE
    stomp *= clamp((worldpos.z - footPos.z) / h, 0.0, 1.0);
#endif
#endif

    return clamp(0.02 * h, 0.0, 1.0) * (harmonics * displace + stomp);
}

void main(void)
{
    vec4 worldPos = osg_ViewMatrixInverse * gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0);
    worldPos.xy += groundcoverDisplacement(worldPos.xyz, originalHeight);
    vec4 viewPos = osg_ViewMatrix * worldPos;

    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);

    gl_Position = viewToClip(viewPos);

    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

    passNormal = gl_Normal.xyz;
    normalToViewMatrix = gl_NormalMatrix;
#if @normalMap
    normalToViewMatrix *= generateTangentSpace(gl_MultiTexCoord7.xyzw, passNormal);
#endif

#if (!PER_PIXEL_LIGHTING || @shadows_enabled)
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
#endif

#if @diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMapUV] * gl_MultiTexCoord@diffuseMapUV).xy;
#endif

#if @normalMap
    normalMapUV = (gl_TextureMatrix[@normalMapUV] * gl_MultiTexCoord@normalMapUV).xy;
#endif

#if PER_PIXEL_LIGHTING
    passViewPos = viewPos.xyz;
#else
    vec3 diffuseLight, ambientLight, specularLight;
    vec3 unusedShadowSpecular;
    doLighting(viewPos.xyz, viewNormal, gl_FrontMaterial.shininess, diffuseLight, ambientLight, specularLight, shadowDiffuseLighting, unusedShadowSpecular);
    passLighting = diffuseLight + ambientLight;
    clampLightingResult(passLighting);
#endif

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}
