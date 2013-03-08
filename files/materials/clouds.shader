#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        shOutput(float, alphaFade)

    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
            alphaFade = (shInputPosition.z <= 200.f) ? ((shInputPosition.z <= 100.f) ? 0.0 : 0.25) : 1.0;
    }

#else

    SH_BEGIN_PROGRAM
		shInput(float2, UV)
                shInput(float, alphaFade)
        
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
        
        shOutputColour(0) = float4(cloudColour, 1) * albedo * float4(1,1,1, cloudOpacity * alphaFade);
    }

#endif
