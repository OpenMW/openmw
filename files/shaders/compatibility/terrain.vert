#version 120

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#include "lib/core/vertex.h.glsl"
varying vec2 uv;
varying float euclideanDepth;
varying float linearDepth;

#define PER_PIXEL_LIGHTING (@normalMap || @specularMap || @forcePPL)

#if !PER_PIXEL_LIGHTING
centroid varying vec3 shadedLighting;
centroid varying vec3 shadedSpecular;
centroid varying vec3 passLighting;
centroid varying vec3 passSpecular;
#include "lib/light/clamp.glsl"
#endif

varying vec3 passViewPos;
varying vec3 passNormal;

#include "vertexcolors.glsl"
#include "shadows_vertex.glsl"
#include "compatibility/normals.glsl"

#include "lib/view/depth.glsl"

void main(void)
{
    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;
    euclideanDepth = length(viewPos.xyz);
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);

    passColor = gl_Color;
    passNormal = gl_Normal.xyz;
    passViewPos = viewPos.xyz;
    normalToViewMatrix = gl_NormalMatrix;

#if @normalMap
    mat3 tbnMatrix = generateTangentSpace(vec4(1.0, 0.0, 0.0, 1.0), passNormal);
    tbnMatrix[0] = -normalize(cross(tbnMatrix[2], tbnMatrix[1])); // our original tangent was not at a 90 degree angle to the normal, so we need to rederive it
    normalToViewMatrix *= tbnMatrix;
#endif

#if !PER_PIXEL_LIGHTING || @shadows_enabled
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
#endif

#if !PER_PIXEL_LIGHTING
    float shininess = max(1e-4, gl_FrontMaterial.shininess);
    vec3 viewDir = passViewPos / euclideanDepth;
    vec3 diffuseColor = getDiffuseColor().rgb;
    vec3 ambientColor = getAmbientColor().rgb;
    vec3 emissionColor = getEmissionColor().rgb;
    vec3 specularColor = getSpecularColor().rgb;

    vec3 sunDiffuse, sunAmbient, sunSpecular, pointDiffuse, pointAmbient, pointSpecular;
    directionalLighting(viewDir, viewNormal, shininess, sunDiffuse, sunAmbient, sunSpecular);
    pointLighting(clipToScreen(gl_Position), viewDir, passViewPos, viewNormal, shininess, pointDiffuse, pointAmbient, pointSpecular);
    shadedLighting = diffuseColor * pointDiffuse + ambientColor * (pointAmbient + sunAmbient) + emissionColor;
    shadedSpecular = specularColor * pointSpecular;
    passLighting = shadedLighting + diffuseColor * sunDiffuse;
    passSpecular = shadedSpecular + specularColor * sunSpecular;
    clampLighting(shadedLighting);
    clampLighting(passLighting);
#endif

    uv = gl_MultiTexCoord0.xy;

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}
