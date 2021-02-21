#version 120

#extension GL_ARB_uniform_buffer_object : enable

#define GROUNDCOVER

attribute vec4 aOffset;
attribute vec3 aRotation;

#if @diffuseMap
varying vec2 diffuseMapUV;
#endif

#if @normalMap
varying vec2 normalMapUV;
varying vec4 passTangent;
#endif

// Other shaders respect forcePPL, but legacy groundcover mods were designed to work with vertex lighting.
// They may do not look as intended with per-pixel lighting, so ignore this setting for now.
#define PER_PIXEL_LIGHTING @normalMap

varying float euclideanDepth;
varying float linearDepth;

#if PER_PIXEL_LIGHTING
varying vec3 passViewPos;
varying vec3 passNormal;
#else
centroid varying vec3 passLighting;
centroid varying vec3 shadowDiffuseLighting;
#endif

#include "shadows_vertex.glsl"
#include "lighting.glsl"

uniform float osg_SimulationTime;
uniform mat4 osg_ViewMatrixInverse;
uniform mat4 osg_ViewMatrix;
uniform float windSpeed;
uniform vec3 playerPos;

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

    float d = length(worldpos - footPos.xyz);
    vec3 stomp = vec3(0.0);
    if (d < 150.0 && d > 0.0)
    {
        stomp = (60.0 / d - 0.4) * (worldpos - footPos.xyz);
    }

    return clamp(0.02 * h, 0.0, 1.0) * (harmonics * displace + stomp.xy);
}

mat4 rotation(in vec3 angle)
{
    float sin_x = sin(angle.x);
    float cos_x = cos(angle.x);
    float sin_y = sin(angle.y);
    float cos_y = cos(angle.y);
    float sin_z = sin(angle.z);
    float cos_z = cos(angle.z);

    return mat4(
        cos_z*cos_y+sin_x*sin_y*sin_z, -sin_z*cos_x, cos_z*sin_y+sin_z*sin_x*cos_y, 0.0,
        sin_z*cos_y+cos_z*sin_x*sin_y, cos_z*cos_x, sin_z*sin_y-cos_z*sin_x*cos_y, 0.0,
        -sin_y*cos_x, sin_x, cos_x*cos_y, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

mat3 rotation3(in mat4 rot4)
{
    return mat3(
        rot4[0].xyz,
        rot4[1].xyz,
        rot4[2].xyz);
}

void main(void)
{
    vec3 position = aOffset.xyz;
    float scale = aOffset.w;

    mat4 rotation = rotation(aRotation);
    vec4 displacedVertex = rotation * scale * gl_Vertex;

    displacedVertex = vec4(displacedVertex.xyz + position, 1.0);

    vec4 worldPos = osg_ViewMatrixInverse * gl_ModelViewMatrix * displacedVertex;
    worldPos.xy += groundcoverDisplacement(worldPos.xyz, gl_Vertex.z);
    vec4 viewPos = osg_ViewMatrix * worldPos;

    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);

    if (length(gl_ModelViewMatrix * vec4(position, 1.0)) > @groundcoverFadeEnd)
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    else
        gl_Position = gl_ProjectionMatrix * viewPos;

    linearDepth = gl_Position.z;

#if (!PER_PIXEL_LIGHTING || @shadows_enabled)
    vec3 viewNormal = normalize((gl_NormalMatrix * rotation3(rotation) * gl_Normal).xyz);
#endif

#if @diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMapUV] * gl_MultiTexCoord@diffuseMapUV).xy;
#endif

#if @normalMap
    normalMapUV = (gl_TextureMatrix[@normalMapUV] * gl_MultiTexCoord@normalMapUV).xy;
    passTangent = gl_MultiTexCoord7.xyzw * rotation;
#endif

#if PER_PIXEL_LIGHTING
    passViewPos = viewPos.xyz;
    passNormal = rotation3(rotation) * gl_Normal.xyz;
#else
    vec3 diffuseLight, ambientLight;
    doLighting(viewPos.xyz, viewNormal, diffuseLight, ambientLight, shadowDiffuseLighting);
    passLighting = diffuseLight + ambientLight;
#endif

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}
