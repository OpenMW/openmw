#include "core.h"

#define MRT @shGlobalSettingBool(mrt_output)

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        shColourInput(float4)
        shOutput(float4, colourPassthrough)

    SH_START_PROGRAM
    {
        colourPassthrough = colour;
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
    }

#else

    SH_BEGIN_PROGRAM
		shInput(float2, UV)
		shInput(float4, colourPassthrough)
#if MRT
        shDeclareMrtOutput(1)
#endif
        
        shSampler2D(diffuseMap1)
        shSampler2D(diffuseMap2)
        
        shUniform(float, cloudBlendFactor)                    @shSharedParameter(cloudBlendFactor)
        shUniform(float, cloudAnimationTimer)                 @shSharedParameter(cloudAnimationTimer)
        shUniform(float, cloudOpacity)                        @shSharedParameter(cloudOpacity)
        shUniform(float3, cloudColour)                        @shSharedParameter(cloudColour)
        
    SH_START_PROGRAM
    {
        // Scroll in y direction
        float2 scrolledUV = UV + float2(0,1) * cloudAnimationTimer * 0.003; 
        
        float4 albedo = shSample(diffuseMap1, scrolledUV) * (1-cloudBlendFactor) + shSample(diffuseMap2, scrolledUV) * cloudBlendFactor;
        
        shOutputColour(0) = colourPassthrough * float4(cloudColour, 1) * albedo * float4(1,1,1, cloudOpacity);

#if MRT
        shOutputColour(1) = float4(1,1,1,1);
#endif
    }

#endif
