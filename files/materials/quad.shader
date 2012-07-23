#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
    SH_START_PROGRAM
    {
        shOutputPosition = shMatrixMult(wvp, shInputPosition);
        UV = uv0;
    }

#else

    SH_BEGIN_PROGRAM
        shInput(float2, UV)
        shSampler2D(SceneBuffer)
    SH_START_PROGRAM
    {
        shOutputColour(0) = shSample(SceneBuffer, UV);
    }

#endif
