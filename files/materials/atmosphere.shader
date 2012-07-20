#include "core.h"

#define MRT @shGlobalSettingBool(mrt_output)

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)

        shColourInput(float4)
        shOutput(float4, colourPassthrough)

    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    colourPassthrough = colour;
    }

#else

    SH_BEGIN_PROGRAM
		shInput(float4, colourPassthrough)
#if MRT
        shDeclareMrtOutput(1)
#endif
        shUniform(float4, atmosphereColour)                   @shSharedParameter(atmosphereColour)

    SH_START_PROGRAM
    {
        shOutputColour(0) = colourPassthrough * atmosphereColour;

#if MRT
        shOutputColour(1) = float4(1,1,1,1);
#endif
    }

#endif
