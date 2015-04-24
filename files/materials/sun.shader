#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
    shUniform(float4x4, world) @shAutoConstant(world, world_matrix)
    shUniform(float4x4, view) @shAutoConstant(view, view_matrix)
shUniform(float4x4, projection) @shAutoConstant(projection, projection_matrix)
        shVertexInput(float2, uv0)
        shOutput(float2, UV)

    SH_START_PROGRAM
    {
        float4x4 viewFixed = view;
#if !SH_GLSL && !SH_GLSLES
        viewFixed[0][3] = 0.0;
        viewFixed[1][3] = 0.0;
        viewFixed[2][3] = 0.0;
#else
        viewFixed[3][0] = 0.0;
        viewFixed[3][1] = 0.0;
        viewFixed[3][2] = 0.0;
#endif
        shOutputPosition = shMatrixMult(projection, shMatrixMult(viewFixed, shMatrixMult(world, shInputPosition)));
        UV = uv0;
    }

#else

    SH_BEGIN_PROGRAM
		shSampler2D(diffuseMap)
		shInput(float2, UV)
        shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
        //shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)

    SH_START_PROGRAM
    {
        shOutputColour(0) = float4(1,1,1,materialDiffuse.a) * shSample(diffuseMap, UV);
    }

#endif
