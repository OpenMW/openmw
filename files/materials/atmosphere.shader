#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)

        shOutput(float, alphaFade)

    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
            alphaFade = shInputPosition.z < 150.0 ? 0.0 : 1.0;
    }

#else

    SH_BEGIN_PROGRAM
                shInput(float, alphaFade)
        shUniform(float4, atmosphereColour)                   @shSharedParameter(atmosphereColour)
        shUniform(float4, horizonColour)                   @shSharedParameter(horizonColour, horizonColour)

    SH_START_PROGRAM
    {
        shOutputColour(0) = alphaFade * atmosphereColour + (1.f - alphaFade) * horizonColour;
    }

#endif
