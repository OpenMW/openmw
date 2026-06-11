#version @version

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#if @lightingMethodClustered
    #extension GL_ARB_shader_storage_buffer_object: require
    #include "lib/light/bindings.glsl"
#else
    #include "lib/light/bindings-legacy.glsl"
#endif

#include "lib/light/util.glsl"
#include "lib/core/fragment.h.glsl"

uniform float near;
uniform vec2 screenRes;
uniform vec3 gridSize;
uniform sampler2D reflectionMap;

vec4 sampleReflectionMap(vec2 uv)
{
    return texture2D(reflectionMap, uv);
}

#if @waterRefraction
uniform sampler2D refractionMap;
uniform sampler2D refractionDepthMap;

vec4 sampleRefractionMap(vec2 uv)
{
    return texture2D(refractionMap, uv);
}

float sampleRefractionDepthMap(vec2 uv)
{
    return texture2D(refractionDepthMap, uv).x;
}

#endif

uniform sampler2D lastShader;

vec4 samplerLastShader(vec2 uv)
{
    return texture2D(lastShader, uv);
}

#if @skyBlending
uniform sampler2D sky;

vec3 sampleSkyColor(vec2 uv)
{
    return texture2D(sky, uv).xyz;
}
#endif

uniform sampler2D opaqueDepthTex;

vec4 sampleOpaqueDepthTex(vec2 uv)
{
    return texture2D(opaqueDepthTex, uv);
}

void doLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal, float shininess, float shadowing, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight) {
    vec3 viewDir = normalize(viewPos);
    shininess = max(shininess, 1e-4);

    diffuseLight = vec3(0.0);
    ambientLight = vec3(0.0);
    specularLight = vec3(0.0);

    calcDirectionalLighting(sun, viewDir, viewNormal, shininess, diffuseLight, ambientLight, specularLight);

    diffuseLight *= shadowing;
    specularLight *= shadowing;

#if @lightingMethodClustered
    LightGrid grid = lightGrid[getClusterTileIndex(screenRes, gridSize, near, screenCoord, viewPos.z)];
    for (uint i = 0u; i < grid.count; ++i) {
        PointLight light = pointLight[lightIndexList[grid.offset + i]];
#else
    for (int i = 0; i < PointLightCount; ++i) {
        PointLight light = PointLight(
            vec4(lcalcPosition(i), 1.0),
            vec4(lcalcDiffuse(i), 0.0),
            vec4(lcalcAmbient(i), 0.0),
            lcalcSpecular(i),
            lcalcConstantAttenuation(i),
            lcalcLinearAttenuation(i),
            lcalcQuadraticAttenuation(i),
            lcalcRadius(i)
        );
#endif

        calcPointLighting(light, viewDir, viewPos, viewNormal, shininess, diffuseLight, ambientLight, specularLight);
    }
}

#if @lightingMethodClustered
vec3 doSpecularLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal) {
    vec3 specular = vec3(0.0);
    vec3 viewDir = normalize(viewPos);
    float shininess = 50.0;

    LightGrid grid = lightGrid[getClusterTileIndex(screenRes, gridSize, near, screenCoord, viewPos.z)];
    for (uint i = 0u; i < grid.count; ++i) {
        PointLight light = pointLight[lightIndexList[grid.offset + i]];

        vec3 lightPos = light.position.xyz - viewPos;
        float lightDistance = length(lightPos);
        vec3 lightDir = lightPos / lightDistance;
        float attenuation = calcAttenuation(light, lightDistance) * clusterFade(viewPos, light.radius);
        specular += light.specular.xyz * specularIntensity(viewNormal, viewDir, shininess, lightDir) * attenuation;
    }

    return specular;
}
#else
vec3 doSpecularLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal) { return vec3(0.0); }
#endif
