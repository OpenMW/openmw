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

#define SPECULAR 1

#define NORMAL_MAP @shPropertyHasValue(normalMap)
#define EMISSIVE_MAP @shPropertyHasValue(emissiveMap)
#define DETAIL_MAP @shPropertyHasValue(detailMap)
#define DIFFUSE_MAP @shPropertyHasValue(diffuseMap)
#define DARK_MAP @shPropertyHasValue(darkMap)
#define SPEC_MAP @shPropertyHasValue(specMap) && SPECULAR

#define PARALLAX @shPropertyBool(use_parallax)
#define PARALLAX_SCALE 0.04
#define PARALLAX_BIAS -0.02

// right now we support 2 UV sets max. implementing them is tedious, and we're probably not going to need more
#define SECOND_UV_SET (@shPropertyString(emissiveMapUVSet) || @shPropertyString(detailMapUVSet) || @shPropertyString(diffuseMapUVSet) || @shPropertyString(darkMapUVSet))

// if normal mapping is enabled, we force pixel lighting
#define VERTEX_LIGHTING (!@shPropertyHasValue(normalMap))

#define UNDERWATER @shGlobalSettingBool(render_refraction)

#define VERTEXCOLOR_MODE @shPropertyString(vertexcolor_mode)

#define VIEWPROJ_FIX @shGlobalSettingBool(viewproj_fix)

#define ENV_MAP @shPropertyBool(env_map)

#define NEED_NORMAL (!VERTEX_LIGHTING || ENV_MAP) || SPECULAR

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

#if NEED_NORMAL
        shOutput(float3, normalPassthrough)
#endif

    // Depth in w
        shOutput(float4, objSpacePositionPassthrough)

#if VERTEXCOLOR_MODE != 0
        shColourInput(float4)
#endif

#if VERTEXCOLOR_MODE != 0 && !VERTEX_LIGHTING
        shOutput(float4, colourPassthrough)
#endif

#if ENV_MAP || VERTEX_LIGHTING
    shUniform(float4x4, worldView) @shAutoConstant(worldView, worldview_matrix)
#endif

#if VERTEX_LIGHTING
    shUniform(float4, lightPosition[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightPosition, light_position_view_space_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightDiffuse[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightDiffuse, light_diffuse_colour_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAttenuation[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightAttenuation, light_attenuation_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAmbient)                    @shAutoConstant(lightAmbient, ambient_light_colour)
#if VERTEXCOLOR_MODE != 2
    shUniform(float4, materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
#endif
    shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
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

#if ENV_MAP || VERTEX_LIGHTING
        float3 viewNormal = normalize(shMatrixMult(worldView, float4(normal.xyz, 0)).xyz);
#endif

#if ENV_MAP
        float3 viewVec = normalize( shMatrixMult(worldView, shInputPosition).xyz);

        float3 r = reflect( viewVec, viewNormal );
        float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
        UV.z = r.x/m + 0.5;
        UV.w = r.y/m + 0.5;
#endif

#if NORMAL_MAP
        tangentPassthrough = tangent.xyz;
#endif
#if NEED_NORMAL
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

        objSpacePositionPassthrough.w = shMatrixMult(fixedWVP, shInputPosition).z;
#else
        objSpacePositionPassthrough.w = shOutputPosition.z;
#endif

#endif

        objSpacePositionPassthrough.xyz = shInputPosition.xyz;

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
                    * max(dot(viewNormal.xyz, lightDir), 0.0);
#else
            lightResult.xyz += materialDiffuse.xyz * lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(viewNormal.xyz, lightDir), 0.0);
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

        lightResult.a *= materialDiffuse.a;

#endif
    }

#else

    // ----------------------------------- FRAGMENT ------------------------------------------

#if UNDERWATER
    #include "underwater.h"
#endif

    SH_BEGIN_PROGRAM
#if DIFFUSE_MAP
        shSampler2D(diffuseMap)
#endif

#if NORMAL_MAP
        shSampler2D(normalMap)
#endif

#if DARK_MAP
        shSampler2D(darkMap)
#endif

#if DETAIL_MAP
        shSampler2D(detailMap)
#endif

#if EMISSIVE_MAP
        shSampler2D(emissiveMap)
#endif

#if ENV_MAP
        shSampler2D(envMap)
        shUniform(float3, env_map_color) @shUniformProperty3f(env_map_color, env_map_color)
#endif

#if SPEC_MAP
        shSampler2D(specMap)
#endif

#if ENV_MAP || SPECULAR || PARALLAX
    shUniform(float3, cameraPosObjSpace) @shAutoConstant(cameraPosObjSpace, camera_position_object_space)
#endif
#if SPECULAR
    shUniform(float3, lightSpec0) @shAutoConstant(lightSpec0, light_specular_colour, 0)
    shUniform(float3, lightPosObjSpace0) @shAutoConstant(lightPosObjSpace0, light_position_object_space, 0)
    shUniform(float, matShininess) @shAutoConstant(matShininess, surface_shininess)
    shUniform(float3, matSpec) @shAutoConstant(matSpec, surface_specular_colour)
#endif

        shInput(float4, UV)

#if NORMAL_MAP
        shInput(float3, tangentPassthrough)
#endif
#if NEED_NORMAL
        shInput(float3, normalPassthrough)
#endif

        shInput(float4, objSpacePositionPassthrough)

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
    shUniform(float4, lightPosition[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightPosition, light_position_view_space_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightDiffuse[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightDiffuse, light_diffuse_colour_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAttenuation[@shGlobalSettingString(num_lights)]) @shAutoConstant(lightAttenuation, light_attenuation_array, @shGlobalSettingString(num_lights))
    shUniform(float4, lightAmbient)                    @shAutoConstant(lightAmbient, ambient_light_colour)
    shUniform(float4x4, worldView) @shAutoConstant(worldView, worldview_matrix)
    #if VERTEXCOLOR_MODE != 2
    shUniform(float4, materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
    #endif
    shUniform(float4, materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
    #if VERTEXCOLOR_MODE != 1
    shUniform(float4, materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)
    #endif
#endif

    SH_START_PROGRAM
    {
        float4 newUV = UV;

#ifdef NEED_DEPTH
        float depthPassthrough = objSpacePositionPassthrough.w;
#endif

#if NEED_NORMAL
        float3 normal = normalPassthrough;
#endif

#if NORMAL_MAP
        float3 binormal = cross(tangentPassthrough.xyz, normal.xyz);
        float3x3 tbn = float3x3(tangentPassthrough.xyz, binormal, normal.xyz);

        #if SH_GLSL
            tbn = transpose(tbn);
        #endif

        float4 normalTex = shSample(normalMap, UV.xy);

        normal = normalize (shMatrixMult( transpose(tbn), normalTex.xyz * 2 - 1 ));
#endif

#if ENV_MAP || SPECULAR || PARALLAX
        float3 eyeDir = normalize(cameraPosObjSpace.xyz - objSpacePositionPassthrough.xyz);
#endif

#if PARALLAX
        float3 TSeyeDir = normalize(shMatrixMult(tbn, eyeDir));

        newUV += (TSeyeDir.xyxy * ( normalTex.a * PARALLAX_SCALE + PARALLAX_BIAS )).xyxy;
#endif

#if DIFFUSE_MAP
    #if @shPropertyString(diffuseMapUVSet)
        float4 diffuse = shSample(diffuseMap, newUV.zw);
    #else
        float4 diffuse = shSample(diffuseMap, newUV.xy);
    #endif
#else
        float4 diffuse = float4(1,1,1,1);
#endif

#if DETAIL_MAP
#if @shPropertyString(detailMapUVSet)
        diffuse *= shSample(detailMap, newUV.zw)*2;
#else
        diffuse *= shSample(detailMap, newUV.xy)*2;
#endif
#endif

#if DARK_MAP
#if @shPropertyString(darkMapUVSet)
        diffuse *= shSample(darkMap, newUV.zw);
#else
        diffuse *= shSample(darkMap, newUV.xy);
#endif
#endif

        shOutputColour(0) = diffuse;

#if !VERTEX_LIGHTING
        float3 viewPos = shMatrixMult(worldView, float4(objSpacePositionPassthrough.xyz,1)).xyz;
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
                    * max(dot(viewNormal.xyz, lightDir), 0.0);
#else
            lightResult.xyz += materialDiffuse.xyz * lightDiffuse[@shIterator].xyz
                    * shSaturate(1.0 / ((lightAttenuation[@shIterator].y) + (lightAttenuation[@shIterator].z * d) + (lightAttenuation[@shIterator].w * d * d)))
                    * max(dot(viewNormal.xyz, lightDir), 0.0);
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

        lightResult.a *= materialDiffuse.a;
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
    float3 worldPos = shMatrixMult(worldMatrix, float4(objSpacePositionPassthrough.xyz,1)).xyz;
#endif

#if UNDERWATER
    float3 waterEyePos = intercept(worldPos, cameraPos.xyz - worldPos, float3(0,0,1), waterLevel);
#endif

#if SHADOWS || SHADOWS_PSSM
        shOutputColour(0) *= (lightResult - float4(directionalResult * (1.0-shadow),0));
#else
        shOutputColour(0) *= lightResult;
#endif

#if EMISSIVE_MAP
        #if @shPropertyString(emissiveMapUVSet)
        shOutputColour(0).xyz += shSample(emissiveMap, newUV.zw).xyz;
        #else
        shOutputColour(0).xyz += shSample(emissiveMap, newUV.xy).xyz;
        #endif
#endif

#if ENV_MAP
        // Everything looks better with fresnel
        float facing = 1.0 - max(abs(dot(-eyeDir, normal)), 0.0);
        float envFactor = shSaturate(0.25 + 0.75 * pow(facing, 1.0));

        shOutputColour(0).xyz += shSample(envMap, UV.zw).xyz * envFactor * env_map_color;
#endif

#if SPECULAR
        float3 light0Dir = normalize(lightPosObjSpace0.xyz);

        float NdotL = max(dot(normal, light0Dir), 0.0);
        float3 halfVec = normalize (light0Dir + eyeDir);

        float shininess = matShininess;
#if SPEC_MAP
        float4 specTex = shSample(specMap, UV.xy);
        shininess *= (specTex.a);
#endif

        float3 specular = pow(max(dot(normal, halfVec), 0.0), shininess) * lightSpec0 * matSpec;
#if SPEC_MAP
        specular *= specTex.xyz;
#else
        specular *= diffuse.a;
#endif

        shOutputColour(0).xyz += specular * shadow;
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
