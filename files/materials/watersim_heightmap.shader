#include "core.h"

#define DAMPING 0.95

#include "watersim_common.h"

    SH_BEGIN_PROGRAM
        shInput(float2, UV)
        shSampler2D(heightPrevSampler)
        shSampler2D(heightCurrentSampler)
        shUniform(float3, previousFrameOffset) @shSharedParameter(previousFrameOffset, previousFrameOffset)
        shUniform(float3, currentFrameOffset) @shSharedParameter(currentFrameOffset, currentFrameOffset)
        shUniform(float4, rippleTextureSize) @shSharedParameter(rippleTextureSize, rippleTextureSize)

    SH_START_PROGRAM
    {
#if !SH_HLSL
		const float3 offset[4] = float3[4](
			float3(-1.0, 0.0, 0.25),
			float3( 1.0, 0.0, 0.25),
			float3( 0.0,-1.0, 0.25),
			float3( 0.0, 1.0, 0.25)
		);	
#else
                const float3 offset[4] = {
                    float3(-1.0, 0.0, 0.25),
                    float3( 1.0, 0.0, 0.25),
                    float3( 0.0,-1.0, 0.25),
                    float3( 0.0, 1.0, 0.25)
                    };
#endif

                float fHeightPrev = DecodeHeightmap(heightPrevSampler, UV.xy + previousFrameOffset.xy + currentFrameOffset.xy);
		
		float fNeighCurrent = 0;
		for ( int i=0; i<4; i++ )
		{
                        float2 vTexcoord = UV + currentFrameOffset.xy + offset[i].xy * rippleTextureSize.xy;
			fNeighCurrent += (DecodeHeightmap(heightCurrentSampler, vTexcoord) * offset[i].z);
		}
		
		float fHeight = fNeighCurrent * 2.0 - fHeightPrev;
		
		fHeight *= DAMPING;
		
		shOutputColour(0) = EncodeHeightmap(fHeight);
    }
    
    
    
    
