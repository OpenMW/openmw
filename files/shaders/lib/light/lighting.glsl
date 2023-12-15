#ifndef LIB_LIGHT_LIGHTING
#define LIB_LIGHT_LIGHTING

#include "lighting_util.glsl"

float calcLambert(vec3 viewNormal, vec3 lightDir, vec3 viewDir)
{
    float lambert = dot(viewNormal, lightDir);
#ifndef GROUNDCOVER
    lambert = max(lambert, 0.0);
#else
    float eyeCosine = dot(viewNormal, viewDir);
    if (lambert < 0.0)
    {
        lambert = -lambert;
        eyeCosine = -eyeCosine;
    }
    lambert *= clamp(-8.0 * (1.0 - 0.3) * eyeCosine + 1.0, 0.3, 1.0);
#endif
    return lambert;
}

#if PER_PIXEL_LIGHTING
void doLighting(vec3 viewPos, vec3 viewNormal, float shadowing, out vec3 diffuseLight, out vec3 ambientLight)
#else
void doLighting(vec3 viewPos, vec3 viewNormal, out vec3 diffuseLight, out vec3 ambientLight, out vec3 shadowDiffuse)
#endif
{
    vec3 viewDir = normalize(viewPos);

    diffuseLight = lcalcDiffuse(0).xyz * calcLambert(viewNormal, normalize(lcalcPosition(0)), viewDir);
    ambientLight = gl_LightModel.ambient.xyz;
#if PER_PIXEL_LIGHTING
    diffuseLight *= shadowing;
#else
    shadowDiffuse = diffuseLight;
    diffuseLight = vec3(0.0);
#endif

    for (int i = @startLight; i < @endLight; ++i)
    {
#if @lightingMethodUBO
        int lightIndex = PointLightIndex[i];
#else
        int lightIndex = i;
#endif
        vec3 lightPos = lcalcPosition(lightIndex) - viewPos;
        float lightDistance = length(lightPos);

        // cull non-FFP point lighting by radius, light is guaranteed to not fall outside this bound with our cutoff
#if !@lightingMethodFFP
        if (lightDistance > lcalcRadius(lightIndex) * 2.0)
            continue;
#endif

        vec3 lightDir = lightPos / lightDistance;

        float illumination = lcalcIllumination(lightIndex, lightDistance);
        ambientLight += lcalcAmbient(lightIndex) * illumination;
        diffuseLight += lcalcDiffuse(lightIndex) * calcLambert(viewNormal, lightDir, viewDir) * illumination;
    }
}

float calcSpecIntensity(vec3 viewNormal, vec3 viewDir, float shininess, vec3 lightDir)
{
    if (dot(viewNormal, lightDir) > 0.0)
    {
        vec3 halfVec = normalize(lightDir - viewDir);
        float NdotH = max(dot(viewNormal, halfVec), 0.0);
        return pow(NdotH, shininess);
    }

    return 0.0;
}

vec3 getSpecular(vec3 viewNormal, vec3 viewPos, float shininess, float shadowing)
{
    shininess = max(shininess, 1e-4);
    vec3 viewDir = normalize(viewPos);
    vec3 specularLight = lcalcSpecular(0).xyz * calcSpecIntensity(viewNormal, viewDir, shininess, normalize(lcalcPosition(0)));
    specularLight *= shadowing;

    for (int i = @startLight; i < @endLight; ++i)
    {
#if @lightingMethodUBO
        int lightIndex = PointLightIndex[i];
#else
        int lightIndex = i;
#endif

        vec3 lightPos = lcalcPosition(lightIndex) - viewPos;
        float lightDistance = length(lightPos);

#if !@lightingMethodFFP
        if (lightDistance > lcalcRadius(lightIndex) * 2.0)
            continue;
#endif

        float illumination = lcalcIllumination(lightIndex, lightDistance);
        float intensity = calcSpecIntensity(viewNormal, viewDir, shininess, normalize(lightPos));
        specularLight += lcalcSpecular(lightIndex).xyz * intensity * illumination;
    }

    return specularLight;
}

#endif
