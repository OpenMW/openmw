#include "core.h"
#include "watersim_common.h"

    SH_BEGIN_PROGRAM
        shInput(float2, UV)
        shSampler2D(heightCurrentSampler)
        shUniform(float4, rippleTextureSize) @shSharedParameter(rippleTextureSize, rippleTextureSize)

    SH_START_PROGRAM
    {
#if !SH_HLSL
                float2 offset[4] = float2[4] (
                    float2(-1.0, 0.0),
                    float2( 1.0, 0.0),
                    float2( 0.0,-1.0),
                    float2( 0.0, 1.0)
                );
#else
                float2 offset[4] = {
                    float2(-1.0, 0.0),
                    float2( 1.0, 0.0),
                    float2( 0.0,-1.0),
                    float2( 0.0, 1.0)
                    };
#endif

		float fHeightL = DecodeHeightmap(heightCurrentSampler, UV.xy + offset[0]*rippleTextureSize.xy);
		float fHeightR = DecodeHeightmap(heightCurrentSampler, UV.xy + offset[1]*rippleTextureSize.xy);
		float fHeightT = DecodeHeightmap(heightCurrentSampler, UV.xy + offset[2]*rippleTextureSize.xy);
		float fHeightB = DecodeHeightmap(heightCurrentSampler, UV.xy + offset[3]*rippleTextureSize.xy);
		
		float3 n = float3(fHeightB - fHeightT, fHeightR - fHeightL, 1.0);
		float3 normal = (n + 1.0) * 0.5;
		
		shOutputColour(0) = float4(normal.rgb, 1.0);
    }
