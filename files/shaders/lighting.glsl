#define LIGHTING_MODEL_FFP 0
#define LIGHTING_MODEL_SINGLE_UBO 1
#define LIGHTING_MODEL_PER_OBJECT_UNIFORM 2

#if @lightingModel != LIGHTING_MODEL_FFP
#define getLight LightBuffer

float quickstep(float x)
{
    x = clamp(x, 0.0, 1.0);
    x = 1.0 - x*x;
    x = 1.0 - x*x;
    return x;
}

#if @lightingModel == LIGHTING_MODEL_SINGLE_UBO

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
              sign bit is stored in diffuse alpha component
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

#else

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

#else
#define getLight gl_LightSource
#endif

void perLightSun(out vec3 ambientOut, out vec3 diffuseOut, vec3 viewPos, vec3 viewNormal)
{
#if @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    vec3 lightDir = normalize(getLight[0][0].xyz);
#else
    vec3 lightDir = normalize(getLight[0].position.xyz);
#endif

#if @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    ambientOut = getLight[0][1].xyz;
    vec3 sunDiffuse = getLight[0][2].xyz;
#elif @lightingModel == LIGHTING_MODEL_SINGLE_UBO
    ivec4 data = getLight[0].packedColors;
    ambientOut = unpackRGB(data.y);
    vec3 sunDiffuse = unpackRGB(data.x);
#else
    ambientOut = getLight[0].ambient.xyz;
    vec3 sunDiffuse = getLight[0].diffuse.xyz;
#endif

    float lambert = dot(viewNormal.xyz, lightDir);
#ifndef GROUNDCOVER
    lambert = max(lambert, 0.0);
#else
    float eyeCosine = dot(normalize(viewPos), viewNormal.xyz);
    if (lambert < 0.0)
    {
        lambert = -lambert;
        eyeCosine = -eyeCosine;
    }
    lambert *= clamp(-8.0 * (1.0 - 0.3) * eyeCosine + 1.0, 0.3, 1.0);
#endif
    diffuseOut = sunDiffuse * lambert;
}

void perLightPoint(out vec3 ambientOut, out vec3 diffuseOut, int lightIndex, vec3 viewPos, vec3 viewNormal)
{
#if @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    vec3 lightPos = getLight[lightIndex][0].xyz - viewPos;
#else
    vec3 lightPos = getLight[lightIndex].position.xyz - viewPos;
#endif

    float lightDistance = length(lightPos);

#if @lightingModel != LIGHTING_MODEL_FFP
#if @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    float radius = getLight[lightIndex][3][3];
#else
    float radius = getLight[lightIndex].attenuation.w;
#endif

    if (lightDistance > radius * 2.0)
    {
        ambientOut = vec3(0.0);
        diffuseOut = vec3(0.0);
        return;
    }
#endif

    lightPos = normalize(lightPos);

#if @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    float illumination = clamp(1.0 / (getLight[lightIndex][0].w + getLight[lightIndex][1].w * lightDistance + getLight[lightIndex][2].w * lightDistance * lightDistance), 0.0, 1.0);
    illumination *= 1.0 - quickstep((lightDistance / radius) - 1.0);
    ambientOut = getLight[lightIndex][1].xyz * illumination;
#elif @lightingModel == LIGHTING_MODEL_SINGLE_UBO
    float illumination = clamp(1.0 / (getLight[lightIndex].attenuation.x + getLight[lightIndex].attenuation.y * lightDistance + getLight[lightIndex].attenuation.z * lightDistance * lightDistance), 0.0, 1.0);
    illumination *= 1.0 - quickstep((lightDistance / radius) - 1.0);
    ivec4 data = getLight[lightIndex].packedColors;
    ambientOut = unpackRGB(data.y) * illumination;
#else
    float illumination = clamp(1.0 / (getLight[lightIndex].constantAttenuation + getLight[lightIndex].linearAttenuation * lightDistance + getLight[lightIndex].quadraticAttenuation * lightDistance * lightDistance), 0.0, 1.0);
    ambientOut = getLight[lightIndex].ambient.xyz * illumination;
#endif

    float lambert = dot(viewNormal.xyz, lightPos) * illumination;

#ifndef GROUNDCOVER
    lambert = max(lambert, 0.0);
#else
    float eyeCosine = dot(normalize(viewPos), viewNormal.xyz);
    if (lambert < 0.0)
    {
        lambert = -lambert;
        eyeCosine = -eyeCosine;
    }
    lambert *= clamp(-8.0 * (1.0 - 0.3) * eyeCosine + 1.0, 0.3, 1.0);
#endif

#if @lightingModel == LIGHTING_MODEL_SINGLE_UBO
    diffuseOut =  unpackRGB(data.x) * lambert * float(int(data.w));
#elif @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    diffuseOut = getLight[lightIndex][2].xyz * lambert;
#else
    diffuseOut = getLight[lightIndex].diffuse.xyz * lambert;
#endif
}

#if PER_PIXEL_LIGHTING
void doLighting(vec3 viewPos, vec3 viewNormal, float shadowing, out vec3 diffuseLight, out vec3 ambientLight)
#else
void doLighting(vec3 viewPos, vec3 viewNormal, out vec3 diffuseLight, out vec3 ambientLight, out vec3 shadowDiffuse)
#endif
{
    vec3 ambientOut, diffuseOut;
    // This light gets added a second time in the loop to fix Mesa users' slowdown, so we need to negate its contribution here.
    perLightSun(ambientOut, diffuseOut, viewPos, viewNormal);

#if PER_PIXEL_LIGHTING
    diffuseLight = diffuseOut * shadowing - diffuseOut;
#else
    shadowDiffuse = diffuseOut;
    diffuseLight = -diffuseOut;
#endif
    ambientLight = gl_LightModel.ambient.xyz;

    perLightSun(ambientOut, diffuseOut, viewPos, viewNormal);
    ambientLight += ambientOut;
    diffuseLight += diffuseOut;

#if @lightingModel == LIGHTING_MODEL_FFP
    for (int i=1; i < @maxLights; ++i)
    {
        perLightPoint(ambientOut, diffuseOut, i, viewPos, viewNormal);
#elif @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    for (int i=1; i <= PointLightCount; ++i)
    {
        perLightPoint(ambientOut, diffuseOut, i, viewPos, viewNormal);
#else
    for (int i=0; i < PointLightCount; ++i)
    {
        perLightPoint(ambientOut, diffuseOut, PointLightIndex[i], viewPos, viewNormal);
#endif
        ambientLight += ambientOut;
        diffuseLight += diffuseOut;
    }
}

vec3 getSpecular(vec3 viewNormal, vec3 viewDirection, float shininess, vec3 matSpec)
{
#if @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    vec3 sunDir = getLight[0][0].xyz;
#else
    vec3 sunDir = getLight[0].position.xyz;
#endif

#if @lightingModel == LIGHTING_MODEL_SINGLE_UBO
    vec3 sunSpec = unpackRGB(getLight[0].packedColors.z);
#elif @lightingModel == LIGHTING_MODEL_PER_OBJECT_UNIFORM
    vec3 sunSpec = getLight[0][3].xyz;
#else
    vec3 sunSpec = getLight[0].specular.xyz;
#endif

    vec3 lightDir = normalize(sunDir);
    float NdotL = dot(viewNormal, lightDir);
    if (NdotL <= 0.0)
        return vec3(0.0);
    vec3 halfVec = normalize(lightDir - viewDirection);
    float NdotH = dot(viewNormal, halfVec);
    return pow(max(NdotH, 0.0), max(1e-4, shininess)) * sunSpec * matSpec;
}
