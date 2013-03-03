#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        shOutput(float, fade)

    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
        UV = uv0;

        fade = (shInputPosition.z > 50) ? 1 : 0;
    }

#else

    SH_BEGIN_PROGRAM

        shInput(float2, UV)
        shInput(float, fade)

        shSampler2D(diffuseMap)
        shUniform(float, nightFade)  @shSharedParameter(nightFade) 


    SH_START_PROGRAM
    {
        shOutputColour(0) = shSample(diffuseMap, UV) * float4(1,1,1, nightFade * fade);
    }

#endif
