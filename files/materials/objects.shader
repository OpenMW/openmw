#include "core.h"


#define FOG @shGlobalSettingBool(fog)

#define SHADOWS_PSSM @shGlobalSettingBool(shadows_pssm)
#define SHADOWS @shGlobalSettingBool(shadows)

#if SHADOWS || SHADOWS_PSSM
    #include "shadows.h"
#endif

#if FOG || SHADOWS_PSSM
#define NEED_DEPTH
#endif

#define NORMAL_MAP @shPropertyHasValue(normalMap)
#define EMISSIVE_MAP @shPropertyHasValue(emissiveMap)
#define DETAIL_MAP @shPropertyHasValue(detailMap)

// right now we support 2 UV sets max. implementing them is tedious, and we're probably not going to need more
#define SECOND_UV_SET (@shPropertyString(emissiveMapUVSet) || @shPropertyString(detailMapUVSet))

// if normal mapping is enabled, we force pixel lighting
#define VERTEX_LIGHTING (!@shPropertyHasValue(normalMap))

#define UNDERWATER @shGlobalSettingBool(render_refraction)

#define VERTEXCOLOR_MODE @shPropertyString(vertexcolor_mode)

#define VIEWPROJ_FIX @shGlobalSettingBool(viewproj_fix)

#ifdef SH_VERTEX_SHADER

    // ------------------------------------- VERTEX ---------------------------------------

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)

        shUniform(float4x4, textureMatrix0) @shAutoConstant(textureMatrix0, texture_matrix, 0)

#if (VIEWPROJ_FIX) || (SHADOWS)
    shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
#endif

#if VIEWPROJ_FIX
        shUniform(float4, vpRow2Fix) @shSharedParameter(vpRow2Fix, vpRow2Fix)
        shUniform(float4x4, vpMatrix) @shAutoConstant(vpMatrix, viewproj_matrix)
#endif

        shVertexInput(float2, uv0)
#if SECOND_UV_SET
        shVertexInput(float2, uv1)
#endif
        shOutput(float4, UV)

        shNormalInput(float4)

#if NORMAL_MAP
        shTangentInput(float4)
        shOutput(float3, tangentPassthrough)
#endif

#if !VERTEX_LIGHTING
        shOutput(float3, normalPassthrough)
#endif

#ifdef NEED_DEPTH
        shOutput(float, depthPassthrough)
#endif

        shOutput(float3, objSpacePositionPassthrough)

#if VERTEXCOLOR_MODE != 0
        shColourInput(float4)
#endif

#if VERTEXCOLOR_MODE != 0 && !VERTEX_LIGHTING
        shOutput(float4, colourPassthrough)
#endif

#if VERTEX_LIGHTING
    shUniform(float4, lightPosition[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightPosition, light_position_view_space_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightDiffuse[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightDiffuse, light_diffuse_colour_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAttenuation[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightAttenuation, light_attenuation_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAmbient)                    @shAutoConstant(lightAmbient, ambient_light_colour)
    shUniform(float4x4, worldView) @shAutoConstant(worldView, worldview_matrix)
#if VERTEXCOLOR_MODE != 2
    shUniform(float4, materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
#endif
#if VERTEXCOLOR_MODE != 2
    shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
#endif
#if VERTEXCOLOR_MODE != 1
    shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)
#endif

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
    shOutput(float4, lightResult)
    shOutput(float3, directionalResult)
#endif
    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);

        UV.xy = shMatrixMult (textureMatrix0, float4(uv0,0,1)).xy;
#if SECOND_UV_SET
        UV.zw = uv1;
#endif

#if NORMAL_MAP
        tangentPassthrough = tangent.xyz;
#endif
#if !VERTEX_LIGHTING
        normalPassthrough = normal.xyz;
#endif
#if VERTEXCOLOR_MODE != 0 && !VERTEX_LIGHTING
        colourPassthrough = colour;
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

        objSpacePositionPassthrough = shInputPosition.xyz;

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
        float3 viewPos = shMatrixMult(worldView, shInputPosition).xyz;
        float3 viewNormal = normalize(shMatrixMult(worldView, float4(normal.xyz, 0)).xyz);

        float3 lightDir;
        float d;
        lightResult = float4(0,0,0,1);
        @shForeach(@shGlobalSettingString(num_lights))
            lightDir = lightPosition[@shIterator].xyz - (viewPos * lightPosition[@shIterator].w);
            d = length(lightDir);
            lightDir = normalize(lightDir);


#if VERTEXCOLOR_MODE == 2
            lightResult.xyz += colour.xyz * lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(viewNormal.xyz, lightDir), 0);
#else
            lightResult.xyz += materialDiffuse.xyz * lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(viewNormal.xyz, lightDir), 0);
#endif

#if @shIterator == 0
            directionalResult = lightResult.xyz;
#endif

        @shEndForeach


#if VERTEXCOLOR_MODE == 2
        lightResult.xyz += lightAmbient.xyz * colour.xyz + materialEmissive.xyz;
        lightResult.a *= colour.a;
#endif
#if VERTEXCOLOR_MODE == 1
        lightResult.xyz += lightAmbient.xyz * materialAmbient.xyz + colour.xyz;
#endif
#if VERTEXCOLOR_MODE == 0
        lightResult.xyz += lightAmbient.xyz * materialAmbient.xyz + materialEmissive.xyz;
#endif

#if VERTEXCOLOR_MODE != 2
        lightResult.a *= materialDiffuse.a;
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

#if NORMAL_MAP
        shSampler2D(normalMap)
#endif

#if EMISSIVE_MAP
        shSampler2D(emissiveMap)
#endif

#if DETAIL_MAP
        shSampler2D(detailMap)
#endif

        shInput(float4, UV)

#if NORMAL_MAP
        shInput(float3, tangentPassthrough)
#endif
#if !VERTEX_LIGHTING
        shInput(float3, normalPassthrough)
#endif

#ifdef NEED_DEPTH
        shInput(float, depthPassthrough)
#endif

        shInput(float3, objSpacePositionPassthrough)

#if VERTEXCOLOR_MODE != 0 && !VERTEX_LIGHTING
        shInput(float4, colourPassthrough)
#endif

#if FOG
        shUniform(float3, fogColour) @shAutoConstant(fogColour, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)
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
    shInput(float4, lightResult)
    shInput(float3, directionalResult)
#else
    shUniform(float, lightCount) @shAutoConstant(lightCount, light_count)
    shUniform(float4, lightPosition[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightPosition, light_position_view_space_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightDiffuse[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightDiffuse, light_diffuse_colour_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAttenuation[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightAttenuation, light_attenuation_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAmbient)                    @shAutoConstant(lightAmbient, ambient_light_colour)
    shUniform(float4x4, worldView) @shAutoConstant(worldView, worldview_matrix)
    #if VERTEXCOLOR_MODE != 2
    shUniform(float4, materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
    #endif
    #if VERTEXCOLOR_MODE != 2
    shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
    #endif
    #if VERTEXCOLOR_MODE != 1
    shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)
    #endif
#endif

    SH_START_PROGRAM
    {
        shOutputColour(0) = shSample(diffuseMap, UV.xy);

#if DETAIL_MAP
#if @shPropertyString(detailMapUVSet)
        shOutputColour(0) *= shSample(detailMap, UV.zw)*2;
#else
        shOutputColour(0) *= shSample(detailMap, UV.xy)*2;
#endif
#endif

#if NORMAL_MAP
        float3 normal = normalPassthrough;
        float3 binormal = cross(tangentPassthrough.xyz, normal.xyz);
        float3x3 tbn = float3x3(tangentPassthrough.xyz, binormal, normal.xyz);

        #if SH_GLSL
            tbn = transpose(tbn);
        #endif

        float3 TSnormal = shSample(normalMap, UV.xy).xyz * 2 - 1;

        normal = normalize (shMatrixMult( transpose(tbn), TSnormal ));
#endif

#if !VERTEX_LIGHTING
        float3 viewPos = shMatrixMult(worldView, float4(objSpacePositionPassthrough,1)).xyz;
        float3 viewNormal = normalize(shMatrixMult(worldView, float4(normal.xyz, 0)).xyz);

        float3 lightDir;
        float d;
        float4 lightResult = float4(0,0,0,1);
        @shForeach(@shGlobalSettingString(num_lights))
            lightDir = lightPosition[@shIterator].xyz - (viewPos * lightPosition[@shIterator].w);
            d = length(lightDir);
            lightDir = normalize(lightDir);

#if VERTEXCOLOR_MODE == 2
            lightResult.xyz += colourPassthrough.xyz * lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(viewNormal.xyz, lightDir), 0);
#else
            lightResult.xyz += materialDiffuse.xyz * lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(viewNormal.xyz, lightDir), 0);
#endif

#if @shIterator == 0
            float3 directionalResult = lightResult.xyz;
#endif

        @shEndForeach


#if VERTEXCOLOR_MODE == 2
        lightResult.xyz += lightAmbient.xyz * colourPassthrough.xyz + materialEmissive.xyz;
        lightResult.a *= colourPassthrough.a;
#endif
#if VERTEXCOLOR_MODE == 1
        lightResult.xyz += lightAmbient.xyz * materialAmbient.xyz + colourPassthrough.xyz;
#endif
#if VERTEXCOLOR_MODE == 0
        lightResult.xyz += lightAmbient.xyz * materialAmbient.xyz + materialEmissive.xyz;
#endif

#if VERTEXCOLOR_MODE != 2
        lightResult.a *= materialDiffuse.a;
#endif
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



#if (UNDERWATER) || (FOG)
    float3 worldPos = shMatrixMult(worldMatrix, float4(objSpacePositionPassthrough,1)).xyz;
#endif

#if UNDERWATER
    float3 waterEyePos = intercept(worldPos, cameraPos.xyz - worldPos, float3(0,0,1), waterLevel);
#endif

#if SHADOWS || SHADOWS_PSSM
        shOutputColour(0) *= (lightResult - float4(directionalResult * (1.0-shadow),0));
#else
        shOutputColour(0) *= lightResult;
#endif

#if FOG
        float fogValue = shSaturate((depthPassthrough - fogParams.y) * fogParams.w);


#if UNDERWATER
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, UNDERWATER_COLOUR, shSaturate(length(waterEyePos-worldPos) / VISIBILITY));
#else
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColour, fogValue);
#endif

#endif

#if EMISSIVE_MAP
        #if @shPropertyString(emissiveMapUVSet)
        shOutputColour(0).xyz += shSample(emissiveMap, UV.zw).xyz;
        #else
        shOutputColour(0).xyz += shSample(emissiveMap, UV.xy).xyz;
        #endif
#endif

        // prevent negative colour output (for example with negative lights)
        shOutputColour(0).xyz = max(shOutputColour(0).xyz, float3(0,0,0));
    }

#endif
