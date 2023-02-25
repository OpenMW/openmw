#version 120

#include "lib/core/vertex.h.glsl"

#if @diffuseMap
varying vec2 diffuseMapUV;
#endif

#if @radialFog
varying float euclideanDepth;
#else
varying float linearDepth;
#endif

uniform bool useFalloff;
uniform vec4 falloffParams;

varying float passFalloff;

#include "vertexcolors.glsl"
#include "lib/view/depth.glsl"

void main(void)
{
    gl_Position = modelToClip(gl_Vertex);

    vec4 viewPos = modelToView(gl_Vertex);
    gl_ClipVertex = viewPos;
#if @radialFog
    euclideanDepth = length(viewPos.xyz);
#else
    linearDepth = getLinearDepth(gl_Position.z, viewPos.z);
#endif

#if @diffuseMap
    diffuseMapUV = (gl_TextureMatrix[@diffuseMapUV] * gl_MultiTexCoord@diffuseMapUV).xy;
#endif

    passColor = gl_Color;
    if (useFalloff)
    {
        vec3 viewNormal = gl_NormalMatrix * normalize(gl_Normal.xyz);
        vec3 viewDir = normalize(viewPos.xyz);
        float viewAngle = abs(dot(viewNormal, viewDir));
        passFalloff = smoothstep(falloffParams.x, falloffParams.y, viewAngle);

        float startOpacity = min(falloffParams.z, 1.0);
        float stopOpacity = max(falloffParams.w, 0.0);

        passFalloff = mix(startOpacity, stopOpacity, passFalloff);
    }
    else
    {
        passFalloff = 1.0;
    }
}
