#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
    SH_START_PROGRAM
    {
        shOutputPosition = shMatrixMult(wvp, shInputPosition);
    }

#else

    SH_BEGIN_PROGRAM
        shUniform(float3, viewportBackground) @shSharedParameter(viewportBackground)
        shDeclareMrtOutput(1)
    SH_START_PROGRAM
    {
        shOutputColour(0) = float4(viewportBackground, 1);
        shOutputColour(1) = float4(1,1,1,1);
    }

#endif
