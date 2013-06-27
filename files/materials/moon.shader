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
#if !SH_GLSL
        viewFixed[0][3] = 0;
        viewFixed[1][3] = 0;
        viewFixed[2][3] = 0;
#else
        viewFixed[3][0] = 0;
        viewFixed[3][1] = 0;
        viewFixed[3][2] = 0;
#endif
        shOutputPosition = shMatrixMult(projection, shMatrixMult(viewFixed, shMatrixMult(world, shInputPosition)));
        UV = uv0;
    }

#else

    SH_BEGIN_PROGRAM
		shSampler2D(diffuseMap)
		shSampler2D(alphaMap)
		shInput(float2, UV)
        shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
        shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)
        
        shUniform(float4, atmosphereColour)                   @shSharedParameter(atmosphereColour)

    SH_START_PROGRAM
    {
        
        float4 tex = shSample(diffuseMap, UV);
        
        shOutputColour(0) = float4(materialEmissive.xyz, 1) * tex;
        
        shOutputColour(0).a = shSample(alphaMap, UV).a * materialDiffuse.a;
        
        shOutputColour(0).rgb += (1-tex.a) * shOutputColour(0).a * atmosphereColour.rgb; //fill dark side of moon with atmosphereColour
        shOutputColour(0).rgb += (1-materialDiffuse.a) * atmosphereColour.rgb; //fade bump

    }

#endif
