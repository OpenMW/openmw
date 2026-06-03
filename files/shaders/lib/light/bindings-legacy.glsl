#include "lib/light/struct.glsl"

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

float lcalcConstantAttenuation(int lightIndex)
{
    return LightBuffer[lightIndex][0].w;
}

float lcalcLinearAttenuation(int lightIndex)
{
    return LightBuffer[lightIndex][1].w;
}

float lcalcQuadraticAttenuation(int lightIndex)
{
    return LightBuffer[lightIndex][2].w;
}

float lcalcRadius(int lightIndex)
{
    return LightBuffer[lightIndex][3].w;
}

vec3 lcalcPosition(int lightIndex)
{
    return LightBuffer[lightIndex][0].xyz;
}

vec3 lcalcDiffuse(int lightIndex)
{
    return LightBuffer[lightIndex][2].xyz;
}

vec3 lcalcAmbient(int lightIndex)
{
    return LightBuffer[lightIndex][1].xyz;
}

vec4 lcalcSpecular(int lightIndex)
{
    return LightBuffer[lightIndex][3];
}

uniform DirectionalLight sun;
