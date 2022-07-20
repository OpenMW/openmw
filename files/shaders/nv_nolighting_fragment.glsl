#version 120
#pragma import_defines(FORCE_OPAQUE)

#if @useGPUShader4
    #extension GL_EXT_gpu_shader4: require
#endif

#if @diffuseMap
uniform sampler2D diffuseMap;
varying vec2 diffuseMapUV;
#endif

varying float euclideanDepth;
varying float linearDepth;

uniform bool useFalloff;
uniform vec2 screenRes;

varying float passFalloff;

#include "vertexcolors.glsl"
#include "alpha.glsl"
#include "fog.glsl"

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

#if defined(FORCE_OPAQUE) && FORCE_OPAQUE
    gl_FragData[0].a = 1.0;
#endif

    gl_FragData[0] = applyFogAtDist(gl_FragData[0], euclideanDepth, linearDepth);
}
