#include "core.h"

#define IS_FIRST_PASS (@shPropertyString(pass_index) == 0)

#define FOG (@shGlobalSettingBool(fog) && !@shPropertyBool(render_composite_map))

#define SHADOWS_PSSM @shGlobalSettingBool(shadows_pssm)
#define SHADOWS @shGlobalSettingBool(shadows)

#if SHADOWS || SHADOWS_PSSM
#include "shadows.h"
#endif

#define NUM_LAYERS @shPropertyString(num_layers)

#if FOG || SHADOWS_PSSM
#define NEED_DEPTH 1
#endif

#define UNDERWATER @shGlobalSettingBool(render_refraction)

#define VIEWPROJ_FIX @shGlobalSettingBool(viewproj_fix)

#define RENDERCMP @shPropertyBool(render_composite_map)

#define LIGHTING !RENDERCMP

#define COMPOSITE_MAP @shPropertyBool(display_composite_map)

#define NORMAL_MAP @shPropertyBool(normal_map_enabled)
#define PARALLAX @shPropertyBool(parallax_enabled)

#define VERTEX_LIGHTING (!NORMAL_MAP)

#define PARALLAX_SCALE 0.04
#define PARALLAX_BIAS -0.02

// This is just for the permutation handler
#define NORMAL_MAPS @shPropertyString(normal_maps)

#if NEED_DEPTH
@shAllocatePassthrough(1, depth)
#endif

@shAllocatePassthrough(2, UV)

@shAllocatePassthrough(3, worldPos)

#if LIGHTING
@shAllocatePassthrough(3, normalPassthrough)
#if VERTEX_LIGHTING
@shAllocatePassthrough(3, lightResult)
@shAllocatePassthrough(3, directionalResult)
#else
@shAllocatePassthrough(3, colourPassthrough)
#endif

#if SHADOWS
@shAllocatePassthrough(4, lightSpacePos0)
#endif
#if SHADOWS_PSSM
@shForeach(3)
    @shAllocatePassthrough(4, lightSpacePos@shIterator)
@shEndForeach
#endif
#endif

#ifdef SH_VERTEX_SHADER

    // ------------------------------------- VERTEX ---------------------------------------

    SH_BEGIN_PROGRAM
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
        shUniform(float4x4, viewProjMatrix) @shAutoConstant(viewProjMatrix, viewproj_matrix)
        
#if VIEWPROJ_FIX
        shUniform(float4, vpRow2Fix) @shSharedParameter(vpRow2Fix, vpRow2Fix)
#endif
        
        shVertexInput(float2, uv0)

#if LIGHTING
        shNormalInput(float4)
        shColourInput(float4)

#if VERTEX_LIGHTING
        shUniform(float4, lightPosition[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightPosition, light_position_object_space_array, @shGlobalSettingString(num_lights))
        shUniform(float4, lightDiffuse[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightDiffuse, light_diffuse_colour_array, @shGlobalSettingString(num_lights))
        shUniform(float4, lightAttenuation[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightAttenuation, light_attenuation_array, @shGlobalSettingString(num_lights))
        shUniform(float4, lightAmbient)                    @shAutoConstant(lightAmbient, ambient_light_colour)
#endif

#if SHADOWS
        shUniform(float4x4, texViewProjMatrix0) @shAutoConstant(texViewProjMatrix0, texture_viewproj_matrix)
#endif

#if SHADOWS_PSSM
    @shForeach(3)
        shUniform(float4x4, texViewProjMatrix@shIterator) @shAutoConstant(texViewProjMatrix@shIterator, texture_viewproj_matrix, @shIterator)
    @shEndForeach
#endif

#endif

        
        @shPassthroughVertexOutputs

    SH_START_PROGRAM
    {
        float4 worldPos = shMatrixMult(worldMatrix, shInputPosition);

        shOutputPosition = shMatrixMult(viewProjMatrix, worldPos);
        
#if NEED_DEPTH
#if VIEWPROJ_FIX
        float4x4 vpFixed = viewProjMatrix;
#if !SH_GLSL
        vpFixed[2] = vpRow2Fix;
#else
        vpFixed[0][2] = vpRow2Fix.x;
        vpFixed[1][2] = vpRow2Fix.y;
        vpFixed[2][2] = vpRow2Fix.z;
        vpFixed[3][2] = vpRow2Fix.w;
#endif

        float4x4 fixedWVP = shMatrixMult(vpFixed, worldMatrix);

        float depth = shMatrixMult(fixedWVP, shInputPosition).z;
        @shPassthroughAssign(depth, depth);
#else
        @shPassthroughAssign(depth, shOutputPosition.z);
#endif

#endif

        @shPassthroughAssign(UV, uv0);
        
        @shPassthroughAssign(worldPos, worldPos.xyz);

#if LIGHTING
        @shPassthroughAssign(normalPassthrough, normal.xyz);
#endif
#if LIGHTING && !VERTEX_LIGHTING
        @shPassthroughAssign(colourPassthrough, colour.xyz);
#endif

#if LIGHTING

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


#if VERTEX_LIGHTING
        // Lighting
        float3 lightDir;
        float d;
        float3 lightResult = float3(0,0,0);
        float3 directionalResult = float3(0,0,0);
        @shForeach(@shGlobalSettingString(num_lights))
            lightDir = lightPosition[@shIterator].xyz - (shInputPosition.xyz * lightPosition[@shIterator].w);
            d = length(lightDir);
            lightDir = normalize(lightDir);


            lightResult.xyz += lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(normal.xyz, lightDir), 0.0);

#if @shIterator == 0
            directionalResult = lightResult.xyz;
#endif
        @shEndForeach
        lightResult.xyz += lightAmbient.xyz;
        lightResult.xyz *= colour.xyz;
        directionalResult.xyz *= colour.xyz;

        @shPassthroughAssign(lightResult, lightResult);
        @shPassthroughAssign(directionalResult, directionalResult);
#endif

#endif
    }

#else

    // ----------------------------------- FRAGMENT ------------------------------------------

#if UNDERWATER
    #include "underwater.h"
#endif

    SH_BEGIN_PROGRAM
    
    
#if COMPOSITE_MAP
        shSampler2D(compositeMap)
#else

@shForeach(@shPropertyString(num_blendmaps))
        shSampler2D(blendMap@shIterator)
@shEndForeach

@shForeach(@shPropertyString(num_layers))
        shSampler2D(diffuseMap@shIterator)
#if @shPropertyBool(use_normal_map_@shIterator)
        shSampler2D(normalMap@shIterator)
#endif
@shEndForeach

#endif
    
#if FOG
        shUniform(float3, fogColour) @shAutoConstant(fogColour, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)
#endif
    
        @shPassthroughFragmentInputs

#if LIGHTING

#if !VERTEX_LIGHTING
shUniform(float4, lightPosition[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightPosition, light_position_array, @shGlobalSettingString(num_lights))
shUniform(float4, lightDiffuse[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightDiffuse, light_diffuse_colour_array, @shGlobalSettingString(num_lights))
shUniform(float4, lightAttenuation[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightAttenuation, light_attenuation_array, @shGlobalSettingString(num_lights))
shUniform(float4, lightAmbient)                    @shAutoConstant(lightAmbient, ambient_light_colour)
shUniform(float4x4, worldView) @shAutoConstant(worldView, worldview_matrix)
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
#endif

#if (UNDERWATER) || (FOG)
        shUniform(float4x4, worldMatrix) @shAutoConstant(worldMatrix, world_matrix)
#endif

#if UNDERWATER
        shUniform(float, waterLevel) @shSharedParameter(waterLevel)
#endif


// For specular
#if LIGHTING
    shUniform(float3, lightSpec0) @shAutoConstant(lightSpec0, light_specular_colour, 0)
    shUniform(float3, lightPos0) @shAutoConstant(lightPos0, light_position, 0)
#endif

shUniform(float4, cameraPos) @shAutoConstant(cameraPos, camera_position)

    SH_START_PROGRAM
    {

#if NEED_DEPTH
        float depth = @shPassthroughReceive(depth);
#endif

        float2 UV = @shPassthroughReceive(UV);
        
        float3 worldPos = @shPassthroughReceive(worldPos);

#if LIGHTING
        float3 normal = @shPassthroughReceive(normalPassthrough);
#endif

#if LIGHTING && !VERTEX_LIGHTING

#if NORMAL_MAP
        // derive the tangent space basis
        float3 tangent = float3(1,0, 0);

        float3 binormal = normalize(cross(tangent, normal));
        tangent = normalize(cross(normal, binormal)); // note, now we need to re-cross to derive tangent again because it wasn't orthonormal

        // derive final matrix
        float3x3 tbn = float3x3(tangent, binormal, normal);
        #if SH_GLSL
        tbn = transpose(tbn);
        #endif
#endif

#endif

#if UNDERWATER
        float3 waterEyePos = intercept(worldPos, cameraPos.xyz - worldPos, float3(0,0,1), waterLevel);
#endif

#if !IS_FIRST_PASS
// Opacity the previous passes should have, i.e. 1 - (opacity of this pass)
float previousAlpha = 1.0;
#endif


shOutputColour(0) = float4(1,1,1,1);

float3 TSnormal = float3(0,0,1);

#if COMPOSITE_MAP
        shOutputColour(0).xyz = shSample(compositeMap, UV).xyz;
#else

        // Layer calculations 
// rescale UV to directly map edge vertices to texel centers - this is
// important to get correct blending at cell transitions
// TODO: parameterize texel size
float2 blendUV = (UV - 0.5) * (16.0 / (16.0+1.0)) + 0.5;
@shForeach(@shPropertyString(num_blendmaps))
        float4 blendValues@shIterator = shSaturate(shSample(blendMap@shIterator, blendUV));
@shEndForeach


        float4 albedo = float4(0,0,0,1);

        float2 layerUV = float2(UV.x, 1.0-UV.y) * 16.0; // Reverse Y, required to get proper tangents
        float2 thisLayerUV;
        float4 normalTex;
        float4 diffuseTex;

        float3 eyeDir = normalize(cameraPos.xyz - worldPos);
#if PARALLAX
        float3 TSeyeDir = normalize(shMatrixMult(tbn, eyeDir));
#endif

@shForeach(@shPropertyString(num_layers))
        thisLayerUV = layerUV;
#if @shPropertyBool(use_normal_map_@shIterator)
        normalTex = shSample(normalMap@shIterator, thisLayerUV);
#if @shIterator == 0 && IS_FIRST_PASS
        TSnormal = normalize(normalTex.xyz * 2.0 - 1.0);
#else
        TSnormal = shLerp(TSnormal, normalTex.xyz * 2.0 - 1.0, blendValues@shPropertyString(blendmap_component_@shIterator));
#endif
#endif

#if @shPropertyBool(use_parallax_@shIterator)
        thisLayerUV += TSeyeDir.xy * ( normalTex.a * PARALLAX_SCALE + PARALLAX_BIAS );
#endif

        diffuseTex = shSample(diffuseMap@shIterator, layerUV);
#if !@shPropertyBool(use_specular_@shIterator)
        diffuseTex.a = 0.0;
#endif

#if @shIterator == 0
albedo = diffuseTex;
#else
albedo = shLerp(albedo, diffuseTex, blendValues@shPropertyString(blendmap_component_@shIterator));
#endif

#if !IS_FIRST_PASS
        previousAlpha *= 1.0-blendValues@shPropertyString(blendmap_component_@shIterator);
#endif


@shEndForeach
        
        shOutputColour(0).rgb *= albedo.xyz;
        
#endif

#if LIGHTING

#if VERTEX_LIGHTING
        // Lighting 
        float3 lightResult = @shPassthroughReceive(lightResult);
        float3 directionalResult = @shPassthroughReceive(directionalResult);
#else

#if NORMAL_MAP
        normal = normalize (shMatrixMult( transpose(tbn), TSnormal ));
#endif

        float3 colour = @shPassthroughReceive(colourPassthrough);
        float3 lightDir;
        float d;
        float3 lightResult = float3(0,0,0);
        @shForeach(@shGlobalSettingString(num_lights))
            lightDir = lightPosition[@shIterator].xyz - (worldPos * lightPosition[@shIterator].w);
            d = length(lightDir);
            lightDir = normalize(lightDir);

            lightResult.xyz += lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(normal.xyz, lightDir), 0.0);
#if @shIterator == 0
            float3 directionalResult = lightResult.xyz;
#endif
        @shEndForeach
        lightResult.xyz += lightAmbient.xyz;
        lightResult.xyz *= colour.xyz;
        directionalResult.xyz *= colour.xyz;
#endif

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

        shOutputColour(0).xyz *= (lightResult - directionalResult * (1.0-shadow));
#endif

#if LIGHTING && !COMPOSITE_MAP
        // Specular
        float3 light0Dir = normalize(lightPos0.xyz);

        float NdotL = max(dot(normal, light0Dir), 0.0);
        float3 halfVec = normalize (light0Dir + eyeDir);

        float3 specular = pow(max(dot(normal, halfVec), 0.0), 32.0) * lightSpec0;
        shOutputColour(0).xyz += specular * (albedo.a) * shadow;
#endif

#if FOG
        float fogValue = shSaturate((depth - fogParams.y) * fogParams.w);
        
        #if UNDERWATER
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, UNDERWATER_COLOUR, shSaturate(length(waterEyePos-worldPos) / VISIBILITY));
        #else
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColour, fogValue);
        #endif
#endif

        // prevent negative colour output (for example with negative lights)
        shOutputColour(0).xyz = max(shOutputColour(0).xyz, float3(0,0,0));

#if IS_FIRST_PASS
        shOutputColour(0).a = 1.0;
#else
        shOutputColour(0).a = 1.0-previousAlpha;
#endif
    }

#endif
