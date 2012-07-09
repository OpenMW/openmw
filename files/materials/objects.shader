#include "core.h"


#define FOG @shPropertyBool(fog)
#define MRT @shPropertyNotBool(is_transparent) && @shPropertyBool(mrt_output)
#define LIGHTING @shPropertyBool(lighting)

#if FOG || MRT
#define NEED_DEPTH
#endif

#define NUM_LIGHTS 8

#define HAS_VERTEXCOLOR @shPropertyBool(has_vertex_colour)

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4 wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shInput(float2, uv0)
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
        shOutput(float4, colorPassthrough)
#endif
    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
        normalPassthrough = normal.xyz;
#ifdef NEED_DEPTH
        depthPassthrough = shOutputPosition.z;
#endif

#if LIGHTING
        objSpacePositionPassthrough = shInputPosition.xyz;
#endif

#if HAS_VERTEXCOLOR
        colorPassthrough = colour;
#endif
    }

#else

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
        shUniform(float far) @shAutoConstant(far, far_clip_distance)
#endif

#if LIGHTING
        shInput(float3, normalPassthrough)
        shInput(float3, objSpacePositionPassthrough)
        shUniform(float4 lightAmbient)                       @shAutoConstant(lightAmbient, ambient_light_colour)
        //shUniform(float passIteration)                       @shAutoConstant(passIteration, pass_iteration_number)
        shUniform(float4 materialAmbient)                    @shAutoConstant(materialAmbient, surface_ambient_colour)
        shUniform(float4 materialDiffuse)                    @shAutoConstant(materialDiffuse, surface_diffuse_colour)
        shUniform(float4 materialEmissive)                   @shAutoConstant(materialEmissive, surface_emissive_colour)
    @shForeach(NUM_LIGHTS)
        shUniform(float4 lightPosObjSpace@shIterator)        @shAutoConstant(lightPosObjSpace@shIterator, light_position_object_space, @shIterator)
        shUniform(float4 lightAttenuation@shIterator)        @shAutoConstant(lightAttenuation@shIterator, light_attenuation, @shIterator)
        shUniform(float4 lightDiffuse@shIterator)            @shAutoConstant(lightDiffuse@shIterator, light_diffuse_colour, @shIterator)
    @shEndForeach
#endif
        
#if FOG
        shUniform(float3 fogColor) @shAutoConstant(fogColor, fog_colour)
        shUniform(float4 fogParams) @shAutoConstant(fogParams, fog_params)
#endif

#ifdef HAS_VERTEXCOLOR
        shInput(float4, colorPassthrough)
#endif
    SH_START_PROGRAM
    {
        shOutputColor(0) = shSample(diffuseMap, UV);

#if LIGHTING
        float3 normal = normalize(normalPassthrough);
        float3 lightDir, diffuse;
        float d;
        float3 ambient = materialAmbient.xyz * lightAmbient.xyz;
    
    @shForeach(NUM_LIGHTS)
    
        lightDir = lightPosObjSpace@shIterator.xyz - (objSpacePositionPassthrough.xyz * lightPosObjSpace@shIterator.w);
        d = length(lightDir);
        
        lightDir = normalize(lightDir);

        diffuse += materialDiffuse.xyz * lightDiffuse@shIterator.xyz * (1.0 / ((lightAttenuation@shIterator.y) + (lightAttenuation@shIterator.z * d) + (lightAttenuation@shIterator.w * d * d))) * max(dot(normal, lightDir), 0);
    
    @shEndForeach
    
#if HAS_VERTEXCOLOR
        ambient *= colorPassthrough.xyz;
#endif

        shOutputColor(0).xyz *= (ambient + diffuse + materialEmissive.xyz);
#endif


#if HAS_VERTEXCOLOR && !LIGHTING
        shOutputColor(0).xyz *= colorPassthrough.xyz;
#endif

#if FOG
        float fogValue = shSaturate((depthPassthrough - fogParams.y) * fogParams.w);
        shOutputColor(0).xyz = shLerp (shOutputColor(0).xyz, fogColor, fogValue);
#endif

        // prevent negative color output (for example with negative lights)
        shOutputColor(0).xyz = max(shOutputColor(0).xyz, float3(0,0,0));

#if MRT
        shOutputColor(1) = float4(depthPassthrough / far,1,1,1);
#endif
    }

#endif
