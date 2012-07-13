#include "core.h"

#define FOG @shGlobalSettingBool(fog)
#define MRT @shGlobalSettingBool(mrt_output)


@shAllocatePassthrough(1, depth)

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

    }

#else

    SH_BEGIN_PROGRAM
    
    
    
        @shPassthroughFragmentInputs
    
#if MRT
        shDeclareMrtOutput(1)
#endif



    SH_START_PROGRAM
    {

        float depth = @shPassthroughReceive(depth);
        
        shOutputColour(0) = float4(1,0,0,1);


#if MRT
        //shOutputColour(1) = float4(1,1,1,1);
#endif
    }

#endif
