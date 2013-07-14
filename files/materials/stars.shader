#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
    shUniform(float4x4, view) @shAutoConstant(view, view_matrix)
shUniform(float4x4, projection) @shAutoConstant(projection, projection_matrix)

        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        shOutput(float, fade)

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
