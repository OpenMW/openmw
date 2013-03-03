#include "core.h"


#define FOG @shGlobalSettingBool(fog)
#define LIGHTING @shGlobalSettingBool(lighting)

#define SHADOWS_PSSM LIGHTING && @shGlobalSettingBool(shadows_pssm)
#define SHADOWS LIGHTING && @shGlobalSettingBool(shadows)

#if SHADOWS || SHADOWS_PSSM
    #include "shadows.h"
#endif

#if FOG || SHADOWS_PSSM
#define NEED_DEPTH
#endif


#define UNDERWATER @shGlobalSettingBool(render_refraction)


#define HAS_VERTEXCOLOR @shPropertyBool(has_vertex_colour)

#define VERTEX_LIGHTING 1

#define VIEWPROJ_FIX @shGlobalSettingBool(viewproj_fix)

#ifdef SH_VERTEX_SHADER

    // ------------------------------------- VERTEX ---------------------------------------

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)

#if (VIEWPROJ_FIX) || (SHADOWS)
    shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
#endif

#if VIEWPROJ_FIX
        shUniform(float4, vpRow2Fix) @shSharedParameter(vpRow2Fix, vpRow2Fix)
        shUniform(float4x4, vpMatrix) @shAutoConstant(vpMatrix, viewproj_matrix)
#endif

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
#if !VERTEX_LIGHTING
        shOutput(float4, colourPassthrough)
#endif
#endif

#if VERTEX_LIGHTING
    shUniform(float, lightCount) @shAutoConstant(lightCount, light_count)
    shUniform(float4, lightPosition[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightPosition, light_position_object_space_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightDiffuse[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightDiffuse, light_diffuse_colour_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAttenuation[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightAttenuation, light_attenuation_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAmbient)                    @shAutoConstant(lightAmbient, ambient_light_colour)
#if !HAS_VERTEXCOLOUR
    shUniform(float4, materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
#endif
    shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
    shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)

#endif

#if SHADOWS
        shOutput(float4, lightSpacePos0)
        shUniform(float4x4, texViewProjMatrix0) @shAutoConstant(texViewProjMatrix0, texture_viewproj_matrix)
#endif

#if SHADOWS_PSSM
    @shForeach(3)
        shOutput(float4, lightSpacePos@shIterator)
        shUniform(float4x4, texViewProjMatrix@shIterator) @shAutoConstant(texViewProjMatrix@shIterator, texture_viewproj_matrix, @shIterator)
    @shEndForeach
#if !VIEWPROJ_FIX
    shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
#endif
#endif

#if VERTEX_LIGHTING
    shOutput(float3, lightResult)
    shOutput(float3, directionalResult)
#endif
    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
#if LIGHTING
        normalPassthrough = normal.xyz;
#endif

#ifdef NEED_DEPTH


#if VIEWPROJ_FIX
        float4x4 vpFixed = vpMatrix;
#if !SH_GLSL
        vpFixed[2] = vpRow2Fix;
#else
        vpFixed[0][2] = vpRow2Fix.x;
        vpFixed[1][2] = vpRow2Fix.y;
        vpFixed[2][2] = vpRow2Fix.z;
        vpFixed[3][2] = vpRow2Fix.w;
#endif

        float4x4 fixedWVP = shMatrixMult(vpFixed, worldMatrix);

        depthPassthrough = shMatrixMult(fixedWVP, shInputPosition).z;
#else
        depthPassthrough = shOutputPosition.z;
#endif

#endif

#if LIGHTING
        objSpacePositionPassthrough = shInputPosition.xyz;
#endif

#if HAS_VERTEXCOLOR && !VERTEX_LIGHTING
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


#if VERTEX_LIGHTING
        float3 lightDir;
        float d;
        lightResult = float3(0,0,0);
        @shForeach(@shGlobalSettingString(num_lights))
            lightDir = lightPosition[@shIterator].xyz - (shInputPosition.xyz * lightPosition[@shIterator].w);
            d = length(lightDir);
            lightDir = normalize(lightDir);

            lightResult += materialDiffuse.xyz * lightDiffuse[@shIterator].xyz
                    * (1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(normalize(normal.xyz), normalize(lightDir)), 0);

#if @shIterator == 0
            directionalResult = lightResult;
#endif

        @shEndForeach

#if HAS_VERTEXCOLOR
        // ambient vertex colour tracking, FFP behaviour
        lightResult += lightAmbient.xyz * colour.xyz + materialEmissive.xyz;

#else
        lightResult += lightAmbient.xyz * materialAmbient.xyz + materialEmissive.xyz;
#endif

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

#ifdef NEED_DEPTH
        shInput(float, depthPassthrough)
#endif

#if LIGHTING
        shInput(float3, normalPassthrough)
        shInput(float3, objSpacePositionPassthrough)
        #if !HAS_VERTEXCOLOR
        shUniform(float4, materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
        #endif
        shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
        shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)
        shUniform(float4, lightAmbient)                       @shAutoConstant(lightAmbient, ambient_light_colour)
#if !VERTEX_LIGHTING

    @shForeach(@shGlobalSettingString(num_lights))
        shUniform(float4, lightPosObjSpace@shIterator)        @shAutoConstant(lightPosObjSpace@shIterator, light_position_object_space, @shIterator)
        shUniform(float4, lightAttenuation@shIterator)        @shAutoConstant(lightAttenuation@shIterator, light_attenuation, @shIterator)
        shUniform(float4, lightDiffuse@shIterator)            @shAutoConstant(lightDiffuse@shIterator, light_diffuse_colour, @shIterator)
    @shEndForeach
#endif

#endif
        
#if FOG
        shUniform(float3, fogColour) @shAutoConstant(fogColour, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)
#endif

#if HAS_VERTEXCOLOR && !VERTEX_LIGHTING
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
        shUniform(float, waterEnabled) @shSharedParameter(waterEnabled)
#endif

#if VERTEX_LIGHTING
    shInput(float3, lightResult)
    shInput(float3, directionalResult)
#endif

    SH_START_PROGRAM
    {
        shOutputColour(0) = shSample(diffuseMap, UV);
        
#if LIGHTING
        float3 normal = normalize(normalPassthrough);
        float3 lightDir;
        float3 diffuse = float3(0,0,0);
        float d;
    
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



#if (UNDERWATER) || (FOG)
    float3 worldPos = shMatrixMult(worldMatrix, float4(objSpacePositionPassthrough,1)).xyz;
#endif

#if UNDERWATER
    float3 waterEyePos = intercept(worldPos, cameraPos.xyz - worldPos, float3(0,0,1), waterLevel);
#endif

#if !VERTEX_LIGHTING
    
    @shForeach(@shGlobalSettingString(num_lights))
    
        /// \todo use the array auto params for lights, and use a real for-loop with auto param "light_count" iterations 
        lightDir = lightPosObjSpace@shIterator.xyz - (objSpacePositionPassthrough.xyz * lightPosObjSpace@shIterator.w);
        d = length(lightDir);
        
        lightDir = normalize(lightDir);

#if @shIterator == 0

    #if (SHADOWS || SHADOWS_PSSM)
        diffuse += materialDiffuse.xyz * lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0) * shadow;
        
    #else
        diffuse += materialDiffuse.xyz * lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0);
        
    #endif
    
#else
        diffuse += materialDiffuse.xyz * lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0);
#endif

    @shEndForeach

        lightResult = (ambient + diffuse + materialEmissive.xyz);
#endif

#if SHADOWS
        shOutputColour(0).xyz *= (lightResult - directionalResult * (1.0-shadow));
#else
        shOutputColour(0).xyz *= (lightResult);
#endif

#endif // IF LIGHTING

#if HAS_VERTEXCOLOR && !LIGHTING
        shOutputColour(0).xyz *= colourPassthrough.xyz;
#endif

#if FOG
        float fogValue = shSaturate((depthPassthrough - fogParams.y) * fogParams.w);
        

#if UNDERWATER
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, UNDERWATER_COLOUR, shSaturate(length(waterEyePos-worldPos) / VISIBILITY));
#else
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColour, fogValue);
#endif

#endif

        // prevent negative colour output (for example with negative lights)
        shOutputColour(0).xyz = max(shOutputColour(0).xyz, float3(0,0,0));
    }

#endif
