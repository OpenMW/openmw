#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, view) @shAutoConstant(view, view_matrix)
    shUniform(float4x4, projection) @shAutoConstant(projection, projection_matrix)

        shOutput(float, alphaFade)

    SH_START_PROGRAM
    {
        float4x4 viewFixed = view;
#if !SH_GLSL
        viewFixed[0][3] = 0;
        viewFixed[1][3] = 0;
        viewFixed[2][3] = 0;
#else
        viewFixed[3][0] = 0;
        viewFixed[3][1] = 0;
        viewFixed[3][2] = 0;
#endif
        shOutputPosition = shMatrixMult(projection, shMatrixMult(viewFixed, shInputPosition));
            alphaFade = shInputPosition.z < 150.0 ? 0.0 : 1.0;
    }

#else

    SH_BEGIN_PROGRAM
                shInput(float, alphaFade)
        shUniform(float4, atmosphereColour)                   @shSharedParameter(atmosphereColour)
        shUniform(float4, horizonColour)                   @shSharedParameter(horizonColour, horizonColour)

    SH_START_PROGRAM
    {
        shOutputColour(0) = alphaFade * atmosphereColour + (1.0 - alphaFade) * horizonColour;
    }

#endif
