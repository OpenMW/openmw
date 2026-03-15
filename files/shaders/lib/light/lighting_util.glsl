#ifndef LIB_LIGHTING_UTIL
#define LIB_LIGHTING_UTIL

#include "lib/util/quickstep.glsl"

#if @lightingMethodUBO

const int mask = int(0xff);
const ivec4 shift = ivec4(int(0), int(8), int(16), int(24));

vec3 unpackRGB(int data)
{
    return vec3( (float(((data >> shift.x) & mask)) / 255.0)
                ,(float(((data >> shift.y) & mask)) / 255.0)
                ,(float(((data >> shift.z) & mask)) / 255.0));
}

vec4 unpackRGBA(int data)
{
    return vec4( (float(((data >> shift.x) & mask)) / 255.0)
                ,(float(((data >> shift.y) & mask)) / 255.0)
                ,(float(((data >> shift.z) & mask)) / 255.0)
                ,(float(((data >> shift.w) & mask)) / 255.0));
}

/* Layout:
packedColors: 8-bit unsigned RGB packed as (diffuse, ambient, specular).
              sign bit is stored in unused alpha component
attenuation: constant, linear, quadratic, light radius (as defined in content)
*/
struct LightData
{
    ivec4 packedColors;
    vec4 position;
    vec4 attenuation;
};

uniform int PointLightIndex[@maxLights];
uniform int PointLightCount;

// Defaults to shared layout. If we ever move to GLSL 140, std140 layout should be considered
uniform LightBufferBinding
{
    LightData LightBuffer[@maxLightsInScene];
};

#elif @lightingMethodPerObjectUniform

/* Layout:
--------------------------------------- -----------
|  pos_x  |  ambi_r  |  diff_r  |  spec_r         |
|  pos_y  |  ambi_g  |  diff_g  |  spec_g         |
|  pos_z  |  ambi_b  |  diff_b  |  spec_b         |
|  att_c  |  att_l   |  att_q   |  radius/spec_a  |
 --------------------------------------------------
*/
uniform mat4 LightBuffer[@maxLights];
uniform int PointLightCount;

#endif

float lcalcConstantAttenuation(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][0].w;
#else
    return LightBuffer[lightIndex].attenuation.x;
#endif
}

float lcalcLinearAttenuation(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][1].w;
#else
    return LightBuffer[lightIndex].attenuation.y;
#endif
}

float lcalcQuadraticAttenuation(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][2].w;
#else
    return LightBuffer[lightIndex].attenuation.z;
#endif
}

float lcalcRadius(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][3].w;
#else
    return LightBuffer[lightIndex].attenuation.w;
#endif
}

float lcalcIllumination(int lightIndex, float dist)
{
    float illumination = 1.0 / (lcalcConstantAttenuation(lightIndex) + lcalcLinearAttenuation(lightIndex) * dist + lcalcQuadraticAttenuation(lightIndex) * dist * dist);
#if !@classicFalloff
    // Fade illumination between the radius and the radius doubled to diminish pop-in
    illumination *= 1.0 - quickstep((dist / lcalcRadius(lightIndex)) - 1.0);
#endif
    return illumination;
}

vec3 lcalcPosition(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][0].xyz;
#else
    return LightBuffer[lightIndex].position.xyz;
#endif
}

vec3 lcalcDiffuse(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][2].xyz;
#else
    return unpackRGB(LightBuffer[lightIndex].packedColors.x) * float(LightBuffer[lightIndex].packedColors.w);
#endif
}

vec3 lcalcAmbient(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][1].xyz;
#else
    return unpackRGB(LightBuffer[lightIndex].packedColors.y);
#endif
}

vec4 lcalcSpecular(int lightIndex)
{
#if @lightingMethodPerObjectUniform
    return LightBuffer[lightIndex][3];
#else
    return unpackRGBA(LightBuffer[lightIndex].packedColors.z);
#endif
}

void clampLightingResult(inout vec3 lighting)
{
#if @clamp
    lighting = clamp(lighting, vec3(0.0), vec3(1.0));
#else
    lighting = max(lighting, 0.0);
#endif
}

#endif
