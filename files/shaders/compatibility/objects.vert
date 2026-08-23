#version 120

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#if @diffuseMap
varying vec2 diffuseMapUV;
uniform mat4 texMat@diffuseMapUV;
#endif

#if @darkMap
varying vec2 darkMapUV;
uniform mat4 texMat@darkMapUV;
#endif

#if @detailMap
varying vec2 detailMapUV;
uniform mat4 texMat@detailMapUV;
#endif

#if @decalMap
varying vec2 decalMapUV;
uniform mat4 texMat@decalMapUV;
#endif

#if @emissiveMap
varying vec2 emissiveMapUV;
uniform mat4 texMat@emissiveMapUV;
#endif

#if @normalMap
varying vec2 normalMapUV;
uniform mat4 texMat@normalMapUV;
#endif

#if @envMap
varying vec2 envMapUV;
uniform mat4 texMat@envMapUV;
#endif

#if @bumpMap
varying vec2 bumpMapUV;
uniform mat4 texMat@bumpMapUV;
#endif

#if @specularMap
varying vec2 specularMapUV;
uniform mat4 texMat@specularMapUV;
#endif

#if @glossMap
varying vec2 glossMapUV;
uniform mat4 texMat@glossMapUV;
#endif

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
#if @normalMap || @diffuseParallax
varying vec4 passTangent;
#endif

#include "lib/core/vertex.h.glsl"

#include "lib/material/vertexcolors.glsl"
#include "shadows_vertex.glsl"
#include "compatibility/normals.glsl"
#include "lib/view/depth.glsl"

#if @particleOcclusion
varying vec3 orthoDepthMapCoord;

uniform mat4 depthSpaceMatrix;
uniform mat4 osg_ViewMatrixInverse;
#endif

uniform vec2 screenRes;

centroid varying vec4 passColor;

void main(void)
{
    Material material = getMaterial();

#if @particleOcclusion
    mat4 model = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    orthoDepthMapCoord = ((depthSpaceMatrix * model) * vec4(gl_Vertex.xyz, 1.0)).xyz;
#endif

    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;
    passColor = gl_Color;
    passViewPos = viewPos.xyz;
    passNormal = gl_Normal.xyz;
    normalToViewMatrix = gl_NormalMatrix;

#if @normalMap || @diffuseParallax
    passTangent = gl_MultiTexCoord7.xyzw;
    normalToViewMatrix *= generateTangentSpace(passTangent, passNormal);
#endif

#if @envMap || !PER_PIXEL_LIGHTING || @shadows_enabled
    vec3 viewNormal = normalize(gl_NormalMatrix * passNormal);
#endif

#if @envMap
    vec3 viewVec = normalize(viewPos.xyz);
    vec3 r = reflect( viewVec, viewNormal );
    float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
    envMapUV = vec2(r.x/m + 0.5, r.y/m + 0.5);
#endif

#if @diffuseMap
    diffuseMapUV = (texMat@diffuseMapUV * gl_MultiTexCoord@diffuseMapUV).xy;
#endif

#if @darkMap
    darkMapUV = (texMat@darkMapUV * gl_MultiTexCoord@darkMapUV).xy;
#endif

#if @detailMap
    detailMapUV = (texMat@detailMapUV * gl_MultiTexCoord@detailMapUV).xy;
#endif

#if @decalMap
    decalMapUV = (texMat@decalMapUV * gl_MultiTexCoord@decalMapUV).xy;
#endif

#if @emissiveMap
    emissiveMapUV = (texMat@emissiveMapUV * gl_MultiTexCoord@emissiveMapUV).xy;
#endif

#if @normalMap
    normalMapUV = (texMat@normalMapUV * gl_MultiTexCoord@normalMapUV).xy;
#endif

#if @bumpMap
    bumpMapUV = (texMat@bumpMapUV * gl_MultiTexCoord@bumpMapUV).xy;
#endif

#if @specularMap
    specularMapUV = (texMat@specularMapUV * gl_MultiTexCoord@specularMapUV).xy;
#endif

#if @glossMap
    glossMapUV = (texMat@glossMapUV * gl_MultiTexCoord@glossMapUV).xy;
#endif

#if !PER_PIXEL_LIGHTING
    vec3 emissionColor = getEmissionColor(material, passColor).rgb;
    if (skipLighting(material, passColor))
    {
        shadedLighting = passLighting = emissionColor * material.emissiveMult;
        shadedSpecular = passSpecular = vec3(0.0);
    }
    else
    {
        // Handles edge case of off-screen vertices with clustered shading not being lit due to not mapping to any cluster
        vec2 screenCoord = clamp(clipToScreen(gl_Position), vec2(0.0), screenRes - vec2(1.0));
        float shininess = max(1e-4, material.shininess);
        vec3 viewDir = normalize(passViewPos);
        vec3 diffuseColor = getDiffuseColor(material, passColor).rgb;
        vec3 ambientColor = getAmbientColor(material, passColor).rgb;
        vec3 specularColor = getSpecularColor(material, passColor).rgb;

        vec3 sunDiffuse, sunAmbient, sunSpecular, pointDiffuse, pointAmbient, pointSpecular;
        directionalLighting(viewDir, viewNormal, shininess, sunDiffuse, sunAmbient, sunSpecular);
        pointLighting(screenCoord, viewDir, passViewPos, viewNormal, shininess, pointDiffuse, pointAmbient, pointSpecular);
        shadedLighting = diffuseColor * pointDiffuse + ambientColor * (pointAmbient + sunAmbient) + emissionColor * material.emissiveMult;
        shadedSpecular = specularColor * pointSpecular * material.specStrength;
        passLighting = shadedLighting + diffuseColor * sunDiffuse;
        passSpecular = shadedSpecular + specularColor * sunSpecular * material.specStrength;
    }
    clampLighting(shadedLighting);
    clampLighting(passLighting);
#endif

#if (@shadows_enabled)
    setupShadowCoords(viewPos, viewNormal);
#endif
}
