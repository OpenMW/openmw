#if !@ffpLighting

#include "sun.glsl"

#define getLight PointLights

struct PointLight
{
    vec4 position;
    vec4 diffuse;
    vec4 ambient;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    float radius;
};

uniform int PointLightCount;
uniform int PointLightIndex[@maxLights];

layout(std140) uniform PointLightBuffer
{
    PointLight PointLights[@maxLightsInScene];
};

#else
#define getLight gl_LightSource
#endif

void perLightSun(out vec3 ambientOut, out vec3 diffuseOut, vec3 viewPos, vec3 viewNormal)
{
    vec3 lightDir = @sunDirection.xyz;
    lightDir = normalize(lightDir);

    ambientOut = @sunAmbient.xyz;

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
    diffuseOut = @sunDiffuse.xyz * lambert;
}


uniform float osg_SimulationTime;
void perLightPoint(out vec3 ambientOut, out vec3 diffuseOut, int lightIndex, vec3 viewPos, vec3 viewNormal)
{
    vec4 pos = getLight[lightIndex].position;
    vec3 lightDir = pos.xyz - viewPos;

    float lightDistance = length(lightDir);
    lightDir = normalize(lightDir);

    float illumination = clamp(1.0 / (getLight[lightIndex].constantAttenuation + getLight[lightIndex].linearAttenuation * lightDistance + getLight[lightIndex].quadraticAttenuation * lightDistance * lightDistance), 0.0, 1.0);

// Add an artificial cutoff, otherwise effected objects will be brightly lit and adjacent objects not effected by this light will be dark by contrast
// This causes nasty artifacts, especially with active grid so it is necassary for now. 
#if !@ffpLighting
    float cutoff = getLight[lightIndex].radius * 0.5;
    illumination *= 1.0 - smoothstep(0.0, 1.0, ((lightDistance / cutoff) - 1.0) * 0.887);
    illumination = max(0.0, illumination);
#endif

    ambientOut = getLight[lightIndex].ambient.xyz * illumination;

    float lambert = dot(viewNormal.xyz, lightDir) * illumination;
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

#if @ffpLighting
    diffuseOut = getLight[lightIndex].diffuse.xyz * lambert;
#else
    diffuseOut = (getLight[lightIndex].diffuse.xyz * pos.w) * lambert;
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

#if !@ffpLighting
    perLightSun(ambientOut, diffuseOut, viewPos, viewNormal);
    ambientLight += ambientOut;
    diffuseLight += diffuseOut;
    for (int i=0; i<PointLightCount; ++i)
    {
        perLightPoint(ambientOut, diffuseOut, PointLightIndex[i], viewPos, viewNormal);
#else
    for (int i=0; i<@maxLights; ++i)
    {
        perLightPoint(ambientOut, diffuseOut, i, viewPos, viewNormal);
#endif
        ambientLight += ambientOut;
        diffuseLight += diffuseOut;
    }
}

vec3 getSpecular(vec3 viewNormal, vec3 viewDirection, float shininess, vec3 matSpec)
{
    vec3 lightDir = normalize(@sunDirection.xyz);
    float NdotL = dot(viewNormal, lightDir);
    if (NdotL <= 0.0)
        return vec3(0.0);
    vec3 halfVec = normalize(lightDir - viewDirection);
    float NdotH = dot(viewNormal, halfVec);
    return pow(max(NdotH, 0.0), max(1e-4, shininess)) * @sunSpecular.xyz * matSpec;
}
