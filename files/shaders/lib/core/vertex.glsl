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
#include "lib/core/vertex.h.glsl"

uniform float near;
uniform vec2 screenRes;
uniform vec3 gridSize;
uniform mat4 projectionMatrix;

vec4 modelToClip(vec4 pos)
{
    return projectionMatrix * modelToView(pos);
}

vec4 modelToView(vec4 pos)
{
    return gl_ModelViewMatrix * pos;
}

vec4 viewToClip(vec4 pos)
{
    return projectionMatrix * pos;
}

vec2 clipToScreen(vec4 pos)
{
    return ((pos.xy / pos.w) * 0.5 + 0.5) * screenRes;
}

void doLighting(vec2 screenCoord, vec3 viewPos, vec3 viewNormal, float shininess, out vec3 diffuseLight, out vec3 ambientLight, out vec3 specularLight, out vec3 shadowDiffuse, out vec3 shadowSpecular) {
    vec3 viewDir = normalize(viewPos);
    shininess = max(shininess, 1e-4);

    diffuseLight = vec3(0.0);
    ambientLight = vec3(0.0);
    specularLight = vec3(0.0);
    shadowDiffuse = vec3(0.0);
    shadowSpecular = vec3(0.0);

    calcDirectionalLighting(sun, viewDir, viewNormal, shininess, shadowDiffuse, ambientLight, shadowSpecular);

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
