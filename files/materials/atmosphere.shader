#include "core.h"

#define MRT @shGlobalSettingBool(mrt_output)

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
#if MRT
        shDeclareMrtOutput(1)
#endif
        shUniform(float4, atmosphereColour)                   @shSharedParameter(atmosphereColour)

    SH_START_PROGRAM
    {
        shOutputColour(0) = atmosphereColour * float4(1,1,1,alphaFade);

#if MRT
        shOutputColour(1) = float4(1,1,1,1);
#endif
    }

#endif
