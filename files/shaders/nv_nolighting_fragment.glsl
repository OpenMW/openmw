#version 120

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

uniform bool noAlpha;

#if @radialFog
varying float euclideanDepth;
#else
varying float linearDepth;
#endif

uniform bool useFalloff;

varying float passFalloff;

#include "vertexcolors.glsl"
#include "alpha.glsl"

void main()
{
#if @diffuseMap
    gl_FragData[0] = texture2D(diffuseMap, diffuseMapUV);
    gl_FragData[0].a *= coveragePreservingAlphaScale(diffuseMap, diffuseMapUV);
#else
    gl_FragData[0] = vec4(1.0);
#endif

    gl_FragData[0] *= getDiffuseColor();

    if (useFalloff)
        gl_FragData[0].a *= passFalloff;

    alphaTest();

#if @radialFog
    float fogValue = clamp((euclideanDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#else
    float fogValue = clamp((linearDepth - gl_Fog.start) * gl_Fog.scale, 0.0, 1.0);
#endif

#if @translucentFramebuffer
    if (noAlpha)
        gl_FragData[0].a = 1.0;
#endif

    gl_FragData[0].xyz = mix(gl_FragData[0].xyz, gl_Fog.color.xyz, fogValue);
}
