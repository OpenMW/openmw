#include "core.h"

#define FOG @shGlobalSettingBool(fog)
#define MRT @shGlobalSettingBool(mrt_output)

#define COLOUR_MAP @shPropertyBool(colour_map)


@shAllocatePassthrough(1, depth)
@shAllocatePassthrough(2, UV)

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
        shUniform(float4x4, viewProjMatrix) @shAutoConstant(viewProjMatrix, viewproj_matrix)
        
        shInput(float2, uv0)
        
        @shPassthroughVertexOutputs

    SH_START_PROGRAM
    {


        float4 worldPos = shMatrixMult(worldMatrix, shInputPosition);


        shOutputPosition = shMatrixMult(viewProjMatrix, worldPos);
        
        @shPassthroughAssign(depth, shOutputPosition.z);
        @shPassthroughAssign(UV, uv0);

    }

#else

    SH_BEGIN_PROGRAM
    
    
#if COLOUR_MAP
        shSampler2D(colourMap)
#endif
    
#if FOG
        shUniform(float3, fogColor) @shAutoConstant(fogColor, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)
#endif
    
        @shPassthroughFragmentInputs
    
#if MRT
        shDeclareMrtOutput(1)
        shUniform(float, far) @shAutoConstant(far, far_clip_distance)
#endif



    SH_START_PROGRAM
    {

        float depth = @shPassthroughReceive(depth);
        float2 UV = @shPassthroughReceive(UV);
        
        shOutputColour(0) = float4(1,0,0,1);
        
#if COLOUR_MAP
        shOutputColour(0).rgb *= shSample(colourMap, UV).rgb;
#endif
        
#if FOG
        float fogValue = shSaturate((depth - fogParams.y) * fogParams.w);
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColor, fogValue);
#endif


#if MRT
        shOutputColour(1) = float4(depth / far,1,1,1);
#endif
    }

#endif
