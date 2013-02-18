#include "core.h"

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shVertexInput(float2, uv0)
        shOutput(float2, UV)

    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
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
