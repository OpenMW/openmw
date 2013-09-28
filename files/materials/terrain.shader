#include "core.h"

#define IS_FIRST_PASS 1

#define FOG @shGlobalSettingBool(fog)
#define MRT @shGlobalSettingBool(mrt_output)

#define LIGHTING @shGlobalSettingBool(lighting)

#define SHADOWS_PSSM LIGHTING && @shGlobalSettingBool(shadows_pssm)
#define SHADOWS LIGHTING && @shGlobalSettingBool(shadows)

#if SHADOWS || SHADOWS_PSSM
#include "shadows.h"
#endif

#define COLOUR_MAP @shPropertyBool(colour_map)

#define NUM_LAYERS @shPropertyString(num_layers)

#if MRT || FOG || SHADOWS_PSSM
#define NEED_DEPTH 1
#endif

#define UNDERWATER @shGlobalSettingBool(underwater_effects) && LIGHTING


#if NEED_DEPTH
@shAllocatePassthrough(1, depth)
#endif

@shAllocatePassthrough(2, UV)

#if LIGHTING
@shAllocatePassthrough(3, objSpacePosition)
#endif

#if SHADOWS
@shAllocatePassthrough(4, lightSpacePos0)
#endif
#if SHADOWS_PSSM
@shForeach(3)
    @shAllocatePassthrough(4, lightSpacePos@shIterator)
@shEndForeach
#endif

#ifdef SH_VERTEX_SHADER

    // ------------------------------------- VERTEX ---------------------------------------

    SH_BEGIN_PROGRAM
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
        shUniform(float4x4, viewProjMatrix) @shAutoConstant(viewProjMatrix, viewproj_matrix)
        
        shUniform(float2, lodMorph) @shAutoConstant(lodMorph, custom, 1001)
        
        shVertexInput(float2, uv0)
        shVertexInput(float2, uv1) // lodDelta, lodThreshold
        
#if SHADOWS
        shUniform(float4x4, texViewProjMatrix0) @shAutoConstant(texViewProjMatrix0, texture_viewproj_matrix)
#endif

#if SHADOWS_PSSM
    @shForeach(3)
        shUniform(float4x4, texViewProjMatrix@shIterator) @shAutoConstant(texViewProjMatrix@shIterator, texture_viewproj_matrix, @shIterator)
    @shEndForeach
#endif

        
        @shPassthroughVertexOutputs

    SH_START_PROGRAM
    {


        float4 worldPos = shMatrixMult(worldMatrix, shInputPosition);

        // determine whether to apply the LOD morph to this vertex
        // we store the deltas against all vertices so we only want to apply 
        // the morph to the ones which would disappear. The target LOD which is
        // being morphed to is stored in lodMorph.y, and the LOD at which 
        // the vertex should be morphed is stored in uv.w. If we subtract
        // the former from the latter, and arrange to only morph if the
        // result is negative (it will only be -1 in fact, since after that
        // the vertex will never be indexed), we will achieve our aim.
        // sign(vertexLOD - targetLOD) == -1 is to morph
        float toMorph = -min(0, sign(uv1.y - lodMorph.y));

        // morph
        // this assumes XZ terrain alignment
        worldPos.y += uv1.x * toMorph * lodMorph.x;


        shOutputPosition = shMatrixMult(viewProjMatrix, worldPos);
        
#if NEED_DEPTH
        @shPassthroughAssign(depth, shOutputPosition.z);
#endif

        @shPassthroughAssign(UV, uv0);
        
#if LIGHTING
        @shPassthroughAssign(objSpacePosition, shInputPosition.xyz);
#endif

#if SHADOWS
        float4 lightSpacePos = shMatrixMult(texViewProjMatrix0, shMatrixMult(worldMatrix, shInputPosition));
        @shPassthroughAssign(lightSpacePos0, lightSpacePos);
#endif
#if SHADOWS_PSSM
        float4 wPos = shMatrixMult(worldMatrix, shInputPosition);
        
        float4 lightSpacePos;
    @shForeach(3)
        lightSpacePos = shMatrixMult(texViewProjMatrix@shIterator, wPos);
        @shPassthroughAssign(lightSpacePos@shIterator, lightSpacePos);
    @shEndForeach
#endif

    }

#else

    // ----------------------------------- FRAGMENT ------------------------------------------

#if UNDERWATER
    #include "underwater.h"
#endif

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
        shUniform(float3, fogColour) @shAutoConstant(fogColour, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)
#endif
    
        @shPassthroughFragmentInputs
    
#if MRT
        shDeclareMrtOutput(1)
        shUniform(float, far) @shAutoConstant(far, far_clip_distance)
#endif


#if LIGHTING
        shUniform(float4, lightAmbient)                       @shAutoConstant(lightAmbient, ambient_light_colour)
    @shForeach(@shGlobalSettingString(terrain_num_lights))
        shUniform(float4, lightPosObjSpace@shIterator)        @shAutoConstant(lightPosObjSpace@shIterator, light_position_object_space, @shIterator)
        shUniform(float4, lightAttenuation@shIterator)        @shAutoConstant(lightAttenuation@shIterator, light_attenuation, @shIterator)
        shUniform(float4, lightDiffuse@shIterator)            @shAutoConstant(lightDiffuse@shIterator, light_diffuse_colour, @shIterator)
    @shEndForeach
#endif


#if SHADOWS
        shSampler2D(shadowMap0)
        shUniform(float2, invShadowmapSize0)   @shAutoConstant(invShadowmapSize0, inverse_texture_size, @shPropertyString(shadowtexture_offset))
#endif
#if SHADOWS_PSSM
    @shForeach(3)
        shSampler2D(shadowMap@shIterator)
        shUniform(float2, invShadowmapSize@shIterator)  @shAutoConstant(invShadowmapSize@shIterator, inverse_texture_size, @shIterator(@shPropertyString(shadowtexture_offset)))
    @shEndForeach
    shUniform(float3, pssmSplitPoints)  @shSharedParameter(pssmSplitPoints)
#endif

#if SHADOWS || SHADOWS_PSSM
        shUniform(float4, shadowFar_fadeStart) @shSharedParameter(shadowFar_fadeStart)
#endif


#if UNDERWATER
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
        shUniform(float, waterLevel) @shSharedParameter(waterLevel)
        shUniform(float4, cameraPos) @shAutoConstant(cameraPos, camera_position) 
        shUniform(float4, lightDirectionWS0) @shAutoConstant(lightDirectionWS0, light_position, 0)
        
        shSampler2D(causticMap)
        
		shUniform(float, waterTimer) @shSharedParameter(waterTimer)
        shUniform(float2, waterSunFade_sunHeight) @shSharedParameter(waterSunFade_sunHeight)
        		
		shUniform(float3, windDir_windSpeed) @shSharedParameter(windDir_windSpeed)
#endif


    SH_START_PROGRAM
    {

#if NEED_DEPTH
        float depth = @shPassthroughReceive(depth);
#endif

        float2 UV = @shPassthroughReceive(UV);
        
#if LIGHTING
        float3 objSpacePosition = @shPassthroughReceive(objSpacePosition);

        float3 normal = shSample(normalMap, UV).rgb * 2 - 1;
        normal = normalize(normal);
#endif
        
        
        
        float3 caustics = float3(1,1,1);
#if UNDERWATER

        float3 worldPos = shMatrixMult(worldMatrix, float4(objSpacePosition,1)).xyz;
        float3 waterEyePos = float3(1,1,1);
        // NOTE: this calculation would be wrong for non-uniform scaling
        float4 worldNormal = shMatrixMult(worldMatrix, float4(normal.xyz, 0));
        waterEyePos = intercept(worldPos, cameraPos.xyz - worldPos, float3(0,1,0), waterLevel);
        caustics = getCaustics(causticMap, worldPos, waterEyePos.xyz, worldNormal.xyz, lightDirectionWS0.xyz, waterLevel, waterTimer, windDir_windSpeed);
        if (worldPos.y >= waterLevel)
            caustics = float3(1,1,1);
        


#endif
        
        
        // Layer calculations 
@shForeach(@shPropertyString(num_blendmaps))
        float4 blendValues@shIterator = shSample(blendMap@shIterator, UV);
@shEndForeach

        float3 albedo = float3(0,0,0);
@shForeach(@shPropertyString(num_layers))


#if IS_FIRST_PASS == 1 && @shIterator == 0
        // first layer of first pass doesn't need a blend map
        albedo = shSample(diffuseMap0, UV * 10).rgb;
#else
        albedo = shLerp(albedo, shSample(diffuseMap@shIterator, UV * 10).rgb, blendValues@shPropertyString(blendmap_component_@shIterator));

#endif
@shEndForeach
        
        shOutputColour(0) = float4(1,1,1,1);
        
#if COLOUR_MAP
        shOutputColour(0).rgb *= shSample(colourMap, UV).rgb;
#endif

        shOutputColour(0).rgb *= albedo;
        
        
        
        
        
        
        // Lighting 
        
#if LIGHTING
        // shadows only for the first (directional) light
#if SHADOWS
            float4 lightSpacePos0 = @shPassthroughReceive(lightSpacePos0);
            float shadow = depthShadowPCF (shadowMap0, lightSpacePos0, invShadowmapSize0);
#endif
#if SHADOWS_PSSM
        @shForeach(3)
            float4 lightSpacePos@shIterator = @shPassthroughReceive(lightSpacePos@shIterator);
        @shEndForeach

            float shadow = pssmDepthShadow (lightSpacePos0, invShadowmapSize0, shadowMap0, lightSpacePos1, invShadowmapSize1, shadowMap1, lightSpacePos2, invShadowmapSize2, shadowMap2, depth, pssmSplitPoints);
#endif

#if SHADOWS || SHADOWS_PSSM
            float fadeRange = shadowFar_fadeStart.x - shadowFar_fadeStart.y;
            float fade = 1-((depth - shadowFar_fadeStart.y) / fadeRange);
            shadow = (depth > shadowFar_fadeStart.x) ? 1.0 : ((depth > shadowFar_fadeStart.y) ? 1.0-((1.0-shadow)*fade) : shadow);
#endif

#if !SHADOWS && !SHADOWS_PSSM
            float shadow = 1.0;
#endif



        float3 lightDir;
        float3 diffuse = float3(0,0,0);
        float d;
        
    @shForeach(@shGlobalSettingString(terrain_num_lights))
    
        lightDir = lightPosObjSpace@shIterator.xyz - (objSpacePosition.xyz * lightPosObjSpace@shIterator.w);
        d = length(lightDir);
       
        
        lightDir = normalize(lightDir);

#if @shIterator == 0

    #if (SHADOWS || SHADOWS_PSSM)
        diffuse += lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0) * shadow * caustics;
        
    #else
        diffuse += lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0) * caustics;
        
    #endif
    
#else
        diffuse += lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0);
#endif

    @shEndForeach
    
        shOutputColour(0).xyz *= (lightAmbient.xyz + diffuse);
#endif
    
    
    
        
#if FOG
        float fogValue = shSaturate((depth - fogParams.y) * fogParams.w);
        
        #if UNDERWATER
        // regular fog only if fragment is above water
        if (worldPos.y > waterLevel)
        #endif
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColour, fogValue);
#endif

        // prevent negative colour output (for example with negative lights)
        shOutputColour(0).xyz = max(shOutputColour(0).xyz, float3(0,0,0));
        
#if UNDERWATER
        float fogAmount = (cameraPos.y > waterLevel)
             ? shSaturate(length(waterEyePos-worldPos) / VISIBILITY) 
             : shSaturate(length(cameraPos.xyz-worldPos)/ VISIBILITY);
             
        float3 eyeVec = normalize(cameraPos.xyz-worldPos);
        
        float waterSunGradient = dot(eyeVec, -normalize(lightDirectionWS0.xyz));
        waterSunGradient = shSaturate(pow(waterSunGradient*0.7+0.3,2.0));  
        float3 waterSunColour = float3(0.0,1.0,0.85)*waterSunGradient * 0.5;
        
        float waterGradient = dot(eyeVec, float3(0.0,-1.0,0.0));
        waterGradient = clamp((waterGradient*0.5+0.5),0.2,1.0);
        float3 watercolour = (float3(0.0078, 0.5176, 0.700)+waterSunColour)*waterGradient*2.0;
        float3 waterext = float3(0.6, 0.9, 1.0);//water extinction
        watercolour = shLerp(watercolour*0.3*waterSunFade_sunHeight.x, watercolour, shSaturate(1.0-exp(-waterSunFade_sunHeight.y*SUN_EXT)));
        watercolour = (cameraPos.y <= waterLevel) ? watercolour : watercolour*0.3;
    
    
        float darkness = VISIBILITY*2.0;
        darkness = clamp((waterEyePos.y - waterLevel + darkness)/darkness,0.2,1.0);
        watercolour *= darkness;

        float isUnderwater = (worldPos.y < waterLevel) ? 1.0 : 0.0;
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, watercolour, fogAmount * isUnderwater);
#endif


#if MRT
        shOutputColour(1) = float4(depth / far,1,1,1);
#endif
    }

#endif
