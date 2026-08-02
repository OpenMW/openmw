#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#if @lightingMethodClustered
    #include "lib/light/bindings.glsl"
#else
    #include "lib/light/bindings-legacy.glsl"
#endif

#include "lib/light/util.glsl"

uniform float near;
uniform vec2 screenRes;
uniform vec3 gridSize;

void directionalLighting(vec3 viewDir, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight)
{
    diffuseLight = vec3(0.0);
    ambientLight = vec3(0.0);
    specularLight = vec3(0.0);

    calcDirectionalLighting(sun, viewDir, viewNormal, shininess, diffuseLight, ambientLight, specularLight);
}

void pointLighting(vec2 screenCoord, vec3 viewDir, vec3 viewPos, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight)
{
    diffuseLight = vec3(0.0);
    ambientLight = vec3(0.0);
    specularLight = vec3(0.0);

#if @simpleLighting || (@particle && !@particlePointLighting)
    return;
#endif

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
