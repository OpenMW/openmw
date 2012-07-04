#include "core.h"


#define FOG @shPropertyBool(fog)
#define MRT @shPropertyNotBool(is_transparent)

#if MRT
#define NEED_DEPTH
#endif

#if FOG
#define NEED_DEPTH
#endif

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4 wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shInput(float2, uv0)
        shOutput(float2, UV)
        shNormalInput(float4)
        shOutput(float4, normalPassthrough)
#ifdef NEED_DEPTH
        shOutput(float, depthPassthrough)
#endif
    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
        normalPassthrough = normal;
#ifdef NEED_DEPTH
        depthPassthrough = shOutputPosition.z;
#endif
    }

#else

    SH_BEGIN_PROGRAM
		shSampler2D(diffuseMap)
		shInput(float2, UV)
        shDeclareMrtOutput(1)
        shInput(float4, normalPassthrough)

#ifdef NEED_DEPTH
        shInput(float, depthPassthrough)
#endif
   
        shUniform(float far) @shAutoConstant(far, far_clip_distance)
        
#if FOG
        shUniform(float3 fogColor) @shAutoConstant(fogColor, fog_colour)
        shUniform(float4 fogParams) @shAutoConstant(fogParams, fog_params)

#endif
    SH_START_PROGRAM
    {
        //shOutputColor(0) = float4((normalize(normalPassthrough.xyz)+float3(1.0,1.0,1.0)) / 2.f, 1.0);
        shOutputColor(0) = shSample(diffuseMap, UV);

#if FOG
        float fogValue = shSaturate((depthPassthrough - fogParams.y) * fogParams.w);
        shOutputColor(0).xyz = shLerp (shOutputColor(0).xyz, fogColor, fogValue);
#endif

        shOutputColor(1) = float4(depthPassthrough / far,1,1,1);
    }

#endif
