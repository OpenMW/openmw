#include "core.h"

#define FOG @shGlobalSettingBool(fog)
#define MRT @shGlobalSettingBool(mrt_output)

#define COLOUR_MAP @shPropertyBool(colour_map)

#define NUM_LAYERS @shPropertyString(num_layers)


#define COMPONENT_0 x
#define COMPONENT_1 y
#define COMPONENT_2 z
#define COMPONENT_3 w

#define IS_FIRST_PASS 1



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

        shSampler2D(normalMap) // global normal map
        
@shForeach(@shPropertyString(num_blendmaps))
        shSampler2D(blendMap@shIterator)
@shEndForeach
        
@shForeach(@shPropertyString(num_layers))
        shSampler2D(diffuseMap@shIterator)
@shEndForeach
    
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
        
        float3 normal = shSample(normalMap, UV).rgb * 2 - 1;
        
        // fetch blendmaps
@shForeach(@shPropertyString(num_blendmaps))
        float4 blendValues@shIterator = shSample(blendMap@shIterator, UV);
@shEndForeach

        float3 albedo;
@shForeach(@shPropertyString(num_layers))


#if IS_FIRST_PASS == 1 && @shIterator == 0
        // first layer of first pass doesn't need a blend map
        albedo = shSample(diffuseMap0, UV * 10).rgb;
#else
        #define BLEND_AMOUNT blendValues@shPropertyString(blendmap_index_@shIterator).@shPropertyString(blendmap_component_@shIterator)
        
        albedo = shLerp(albedo, shSample(diffuseMap@shIterator, UV * 10).rgb, BLEND_AMOUNT);
#endif
@shEndForeach
        
        shOutputColour(0) = float4(1,1,1,1);
        
#if COLOUR_MAP
        shOutputColour(0).rgb *= shSample(colourMap, UV).rgb;
#endif

        shOutputColour(0).rgb *= albedo;
        
        
#if FOG
        float fogValue = shSaturate((depth - fogParams.y) * fogParams.w);
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColor, fogValue);
#endif


#if MRT
        shOutputColour(1) = float4(depth / far,1,1,1);
#endif
    }

#endif
