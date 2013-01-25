#include "core.h"


#define FOG @shGlobalSettingBool(fog)
#define MRT @shPropertyNotBool(is_transparent) && @shGlobalSettingBool(mrt_output)
#define LIGHTING @shGlobalSettingBool(lighting)

#define SHADOWS_PSSM LIGHTING && @shGlobalSettingBool(shadows_pssm)
#define SHADOWS LIGHTING && @shGlobalSettingBool(shadows)

#if SHADOWS || SHADOWS_PSSM
    #include "shadows.h"
#endif

#if FOG || MRT || SHADOWS_PSSM
#define NEED_DEPTH
#endif


#define UNDERWATER @shGlobalSettingBool(underwater_effects) && LIGHTING


#define HAS_VERTEXCOLOR @shPropertyBool(has_vertex_colour)

#ifdef SH_VERTEX_SHADER

    // ------------------------------------- VERTEX ---------------------------------------

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        shNormalInput(float4)
#ifdef NEED_DEPTH
        shOutput(float, depthPassthrough)
#endif

#if LIGHTING
        shOutput(float3, normalPassthrough)
        shOutput(float3, objSpacePositionPassthrough)
#endif

#if HAS_VERTEXCOLOR
        shColourInput(float4)
        shOutput(float4, colourPassthrough)
#endif

#if SHADOWS
        shOutput(float4, lightSpacePos0)
        shUniform(float4x4, texViewProjMatrix0) @shAutoConstant(texViewProjMatrix0, texture_viewproj_matrix)
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
#endif

#if SHADOWS_PSSM
    @shForeach(3)
        shOutput(float4, lightSpacePos@shIterator)
        shUniform(float4x4, texViewProjMatrix@shIterator) @shAutoConstant(texViewProjMatrix@shIterator, texture_viewproj_matrix, @shIterator)
    @shEndForeach
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
#endif
    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
#if LIGHTING
        normalPassthrough = normal.xyz;
#endif

#ifdef NEED_DEPTH
        depthPassthrough = shOutputPosition.z;
#endif

#if LIGHTING
        objSpacePositionPassthrough = shInputPosition.xyz;
#endif

#if HAS_VERTEXCOLOR
        colourPassthrough = colour;
#endif

#if SHADOWS
        lightSpacePos0 = shMatrixMult(texViewProjMatrix0, shMatrixMult(worldMatrix, shInputPosition));
#endif
#if SHADOWS_PSSM
        float4 wPos = shMatrixMult(worldMatrix, shInputPosition);
    @shForeach(3)
        lightSpacePos@shIterator = shMatrixMult(texViewProjMatrix@shIterator, wPos);
    @shEndForeach
#endif
    }

#else

    // ----------------------------------- FRAGMENT ------------------------------------------

#if UNDERWATER
    #include "underwater.h"
#endif

    SH_BEGIN_PROGRAM
		shSampler2D(diffuseMap)
		shInput(float2, UV)
#if MRT
        shDeclareMrtOutput(1)
#endif

#ifdef NEED_DEPTH
        shInput(float, depthPassthrough)
#endif

#if MRT
        shUniform(float, far) @shAutoConstant(far, far_clip_distance)
#endif

        shUniform(float, gammaCorrection) @shSharedParameter(gammaCorrection, gammaCorrection)

#if LIGHTING
        shInput(float3, normalPassthrough)
        shInput(float3, objSpacePositionPassthrough)
        shUniform(float4, lightAmbient)                       @shAutoConstant(lightAmbient, ambient_light_colour)
        #if !HAS_VERTEXCOLOR
        shUniform(float4, materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
        #endif
        shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
        shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)
    @shForeach(@shGlobalSettingString(num_lights))
        shUniform(float4, lightPosObjSpace@shIterator)        @shAutoConstant(lightPosObjSpace@shIterator, light_position_object_space, @shIterator)
        shUniform(float4, lightAttenuation@shIterator)        @shAutoConstant(lightAttenuation@shIterator, light_attenuation, @shIterator)
        shUniform(float4, lightDiffuse@shIterator)            @shAutoConstant(lightDiffuse@shIterator, light_diffuse_colour, @shIterator)
    @shEndForeach
#endif
        
#if FOG
        shUniform(float3, fogColour) @shAutoConstant(fogColour, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)
#endif

#if HAS_VERTEXCOLOR
        shInput(float4, colourPassthrough)
#endif

#if SHADOWS
        shInput(float4, lightSpacePos0)
        shSampler2D(shadowMap0)
        shUniform(float2, invShadowmapSize0)   @shAutoConstant(invShadowmapSize0, inverse_texture_size, 1)
#endif
#if SHADOWS_PSSM
    @shForeach(3)
        shInput(float4, lightSpacePos@shIterator)
        shSampler2D(shadowMap@shIterator)
        shUniform(float2, invShadowmapSize@shIterator)  @shAutoConstant(invShadowmapSize@shIterator, inverse_texture_size, @shIterator(1))
    @shEndForeach
    shUniform(float3, pssmSplitPoints)  @shSharedParameter(pssmSplitPoints)
#endif

#if SHADOWS || SHADOWS_PSSM
        shUniform(float4, shadowFar_fadeStart) @shSharedParameter(shadowFar_fadeStart)
#endif

#if (UNDERWATER) || (FOG)
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
        shUniform(float4, cameraPos) @shAutoConstant(cameraPos, camera_position) 
#endif

#if UNDERWATER

        shUniform(float, waterLevel) @shSharedParameter(waterLevel)

        shUniform(float4, lightDirectionWS0) @shAutoConstant(lightDirectionWS0, light_position, 0)
        
        shSampler2D(causticMap)
        
		shUniform(float, waterTimer) @shSharedParameter(waterTimer)
        shUniform(float2, waterSunFade_sunHeight) @shSharedParameter(waterSunFade_sunHeight)
        shUniform(float, waterEnabled) @shSharedParameter(waterEnabled)
        		
		shUniform(float3, windDir_windSpeed) @shSharedParameter(windDir_windSpeed)
#endif

    SH_START_PROGRAM
    {
        shOutputColour(0) = shSample(diffuseMap, UV);
        shOutputColour(0).xyz = gammaCorrectRead(shOutputColour(0).xyz);
        
#if LIGHTING
        float3 normal = normalize(normalPassthrough);
        float3 lightDir;
        float3 diffuse = float3(0,0,0);
        float d;

#if HAS_VERTEXCOLOR
        // ambient vertex colour tracking, FFP behaviour
        float3 ambient = colourPassthrough.xyz * lightAmbient.xyz;
#else
        float3 ambient = materialAmbient.xyz * lightAmbient.xyz;
#endif
    
            // shadows only for the first (directional) light
#if SHADOWS
            float shadow = depthShadowPCF (shadowMap0, lightSpacePos0, invShadowmapSize0);
#endif
#if SHADOWS_PSSM
            float shadow = pssmDepthShadow (lightSpacePos0, invShadowmapSize0, shadowMap0, lightSpacePos1, invShadowmapSize1, shadowMap1, lightSpacePos2, invShadowmapSize2, shadowMap2, depthPassthrough, pssmSplitPoints);
#endif

#if SHADOWS || SHADOWS_PSSM
            float fadeRange = shadowFar_fadeStart.x - shadowFar_fadeStart.y;
            float fade = 1-((depthPassthrough - shadowFar_fadeStart.y) / fadeRange);
            shadow = (depthPassthrough > shadowFar_fadeStart.x) ? 1.0 : ((depthPassthrough > shadowFar_fadeStart.y) ? 1.0-((1.0-shadow)*fade) : shadow);
#endif

#if !SHADOWS && !SHADOWS_PSSM
            float shadow = 1.0;
#endif



        float3 caustics = float3(1,1,1);

#if (UNDERWATER) || (FOG)
    float3 worldPos = shMatrixMult(worldMatrix, float4(objSpacePositionPassthrough,1)).xyz;
#endif

#if UNDERWATER
    float3 waterEyePos = float3(1,1,1);
    // NOTE: this calculation would be wrong for non-uniform scaling
    float4 worldNormal = shMatrixMult(worldMatrix, float4(normal.xyz, 0));
    waterEyePos = intercept(worldPos, cameraPos.xyz - worldPos, float3(0,1,0), waterLevel);
    caustics = getCaustics(causticMap, worldPos, waterEyePos.xyz, worldNormal.xyz, lightDirectionWS0.xyz, waterLevel, waterTimer, windDir_windSpeed);
    if (worldPos.y >= waterLevel || waterEnabled != 1.f)
        caustics = float3(1,1,1);
#endif

    
    @shForeach(@shGlobalSettingString(num_lights))
    
        /// \todo use the array auto params for lights, and use a real for-loop with auto param "light_count" iterations 
        lightDir = lightPosObjSpace@shIterator.xyz - (objSpacePositionPassthrough.xyz * lightPosObjSpace@shIterator.w);
        d = length(lightDir);
        
        lightDir = normalize(lightDir);

#if @shIterator == 0

    #if (SHADOWS || SHADOWS_PSSM)
        diffuse += materialDiffuse.xyz * lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0) * shadow * caustics;
        
    #else
        diffuse += materialDiffuse.xyz * lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0) * caustics;
        
    #endif
    
#else
        diffuse += materialDiffuse.xyz * lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0);
#endif

    @shEndForeach

        shOutputColour(0).xyz *= (ambient + diffuse + materialEmissive.xyz);
#endif


#if HAS_VERTEXCOLOR && !LIGHTING
        shOutputColour(0).xyz *= colourPassthrough.xyz;
#endif

#if FOG
        float fogValue = shSaturate((length(cameraPos.xyz-worldPos) - fogParams.y) * fogParams.w);
        
        #if UNDERWATER
        // regular fog only if fragment is above water
        if (worldPos.y > waterLevel || waterEnabled != 1.f)
        #endif
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, gammaCorrectRead(fogColour), fogValue);
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
        float3 waterSunColour = gammaCorrectRead(float3(0.0,1.0,0.85)) *waterSunGradient * 0.5;
        
        float waterGradient = dot(eyeVec, float3(0.0,-1.0,0.0));
        waterGradient = clamp((waterGradient*0.5+0.5),0.2,1.0);
        float3 watercolour = ( gammaCorrectRead(float3(0.0078, 0.5176, 0.700))+waterSunColour)*waterGradient*2.0;
        watercolour = shLerp(watercolour*0.3*waterSunFade_sunHeight.x, watercolour, shSaturate(1.0-exp(-waterSunFade_sunHeight.y*SUN_EXT)));
        watercolour = (cameraPos.y <= waterLevel) ? watercolour : watercolour*0.3;
    
    
        float darkness = VISIBILITY*2.0;
        darkness = clamp((waterEyePos.y - waterLevel + darkness)/darkness,0.2,1.0);
        watercolour *= darkness;

        float isUnderwater = (worldPos.y < waterLevel) ? 1.0 : 0.0;
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, watercolour, fogAmount * isUnderwater * waterEnabled);
#endif

        shOutputColour(0).xyz = gammaCorrectOutput(shOutputColour(0).xyz);

#if MRT
        shOutputColour(1) = float4(depthPassthrough / far,1,1,1);
#endif

    }

#endif
