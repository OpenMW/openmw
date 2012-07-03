#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4 wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shInput(float2, uv0)
        shOutput(float2, UV)
    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
    }

#else

    SH_BEGIN_PROGRAM
		shSampler2D(diffuseMap)
		shInput(float2, UV)

    SH_START_PROGRAM
    {
       // shOutputColor = float4(1.0, 0.0, 0.0, 1.0);
        shOutputColor = shSample(diffuseMap, UV);
    }

#endif
