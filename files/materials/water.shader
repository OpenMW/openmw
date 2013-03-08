#include "core.h"


#define SIMPLE_WATER @shGlobalSettingBool(simple_water)


#if SIMPLE_WATER
    // --------------------------------------- SIMPLE WATER ---------------------------------------------------   

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp) @shAutoConstant(wvp, worldviewproj_matrix)
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        shOutput(float, depth)

    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
	    depth = shOutputPosition.z;
    }

#else

    SH_BEGIN_PROGRAM
		shSampler2D(animatedTexture)
		shInput(float2, UV)
		shInput(float, depth)

		shUniform(float3, fogColor) @shAutoConstant(fogColor, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)


    SH_START_PROGRAM
    {
        shOutputColour(0).xyz = shSample(animatedTexture, UV * 15).xyz * float3(1.0, 1.0, 1.0);
        shOutputColour(0).w = 0.7;
        
        float fogValue = shSaturate((depth - fogParams.y) * fogParams.w);
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColor, fogValue);
    }

#endif

#else



// Inspired by Blender GLSL Water by martinsh ( http://devlog-martinsh.blogspot.de/2012/07/waterundewater-shader-wip.html )

#define RIPPLES 1
#define REFRACTION @shGlobalSettingBool(refraction)

#ifdef SH_VERTEX_SHADER

    SH_BEGIN_PROGRAM
        shUniform(float4x4, wvp)                @shAutoConstant(wvp, worldviewproj_matrix)
        shVertexInput(float2, uv0)
        shOutput(float2, UV)
        
        shOutput(float3, screenCoordsPassthrough)
        shOutput(float4, position)
        shOutput(float, depthPassthrough)

    SH_START_PROGRAM
    {
	    shOutputPosition = shMatrixMult(wvp, shInputPosition);
	    UV = uv0;
	   
	   
	    #if !SH_GLSL
        float4x4 scalemat = float4x4(   0.5,    0,      0,      0.5,
                                        0,      -0.5,   0,      0.5,
                                        0,      0,      0.5,    0.5,
                                        0,      0,      0,      1   );
        #else                            
        mat4 scalemat = mat4(0.5, 0.0, 0.0, 0.0, 
                         0.0, -0.5, 0.0, 0.0,
                         0.0, 0.0, 0.5, 0.0,
                         0.5, 0.5, 0.5, 1.0);
        #endif
                                        
        float4 texcoordProj = shMatrixMult(scalemat, shOutputPosition);
        screenCoordsPassthrough = float3(texcoordProj.x, texcoordProj.y, texcoordProj.w);
        
        position = shInputPosition;
        
        depthPassthrough = shOutputPosition.z;
    }

#else

    // tweakables ----------------------------------------------------

        #define VISIBILITY 1500.0                   // how far you can look through water

        #define BIG_WAVES_X 0.3                     // strength of big waves
        #define BIG_WAVES_Y 0.3   
                          
        #define MID_WAVES_X 0.3                     // strength of middle sized waves
        #define MID_WAVES_Y 0.15        
        
        #define SMALL_WAVES_X 0.15                  // strength of small waves
        #define SMALL_WAVES_Y 0.1      
        
        #define WAVE_CHOPPYNESS 0.15                // wave choppyness
        #define WAVE_SCALE 75                      // overall wave scale

        #define BUMP 1.5                            // overall water surface bumpiness
        #define REFL_BUMP 0.08                      // reflection distortion amount
        #define REFR_BUMP 0.06                      // refraction distortion amount

        #define SCATTER_AMOUNT 0.3                  // amount of sunlight scattering
        #define SCATTER_COLOUR float3(0.0,1.0,0.95) // colour of sunlight scattering
        
        #define SUN_EXT float3(0.45, 0.55, 0.68)   //sunlight extinction
        
        #define SPEC_HARDNESS 256                   // specular highlights hardness
        

    // ---------------------------------------------------------------



    float fresnel_dielectric(float3 Incoming, float3 Normal, float eta)
    {
        /* compute fresnel reflectance without explicitly computing
           the refracted direction */
        float c = abs(dot(Incoming, Normal));
        float g = eta * eta - 1.0 + c * c;
        float result;

        if(g > 0.0) {
            g = sqrt(g);
            float A =(g - c)/(g + c);
            float B =(c *(g + c)- 1.0)/(c *(g - c)+ 1.0);
            result = 0.5 * A * A *(1.0 + B * B);
        }
        else
            result = 1.0;  /* TIR (no refracted component) */

        return result;
    }

    SH_BEGIN_PROGRAM
		shInput(float2, UV)
		shInput(float3, screenCoordsPassthrough)
		shInput(float4, position)
		shInput(float, depthPassthrough)

            #if RIPPLES
                shUniform(float3, rippleCenter) @shSharedParameter(rippleCenter, rippleCenter)
                shUniform(float, rippleAreaLength) @shSharedParameter(rippleAreaLength, rippleAreaLength)
            #endif
		
		shUniform(float, far) @shAutoConstant(far, far_clip_distance)
	
		shSampler2D(reflectionMap)
#if REFRACTION
		shSampler2D(refractionMap)
#endif
		shSampler2D(depthMap)
		shSampler2D(normalMap)

            #if RIPPLES
                shSampler2D(rippleNormalMap)
                shUniform(float4x4, wMat) @shAutoConstant(wMat, world_matrix)
            #endif
		
		shUniform(float3, windDir_windSpeed) @shSharedParameter(windDir_windSpeed)
		#define WIND_SPEED windDir_windSpeed.z
		#define WIND_DIR windDir_windSpeed.xy
		
		shUniform(float, waterTimer) @shSharedParameter(waterTimer)
		shUniform(float2, waterSunFade_sunHeight) @shSharedParameter(waterSunFade_sunHeight)
		
		shUniform(float4, sunPosition) @shAutoConstant(sunPosition, light_position, 0)
		shUniform(float4, sunSpecular)  @shAutoConstant(sunSpecular, light_specular_colour, 0)
		
		shUniform(float, renderTargetFlipping) @shAutoConstant(renderTargetFlipping, render_target_flipping)
		
		
		shUniform(float3, fogColor) @shAutoConstant(fogColor, fog_colour)
        shUniform(float4, fogParams) @shAutoConstant(fogParams, fog_params)
        
        shUniform(float4, cameraPos) @shAutoConstant(cameraPos, camera_position_object_space)
		

    SH_START_PROGRAM
    {
        float2 screenCoords = screenCoordsPassthrough.xy / screenCoordsPassthrough.z;
        screenCoords.y = (1-shSaturate(renderTargetFlipping))+renderTargetFlipping*screenCoords.y;

	    float2 nCoord = float2(0,0);

      	nCoord = UV * (WAVE_SCALE * 0.05) + WIND_DIR * waterTimer * (WIND_SPEED*0.04);
	    float3 normal0 = 2.0 * shSample(normalMap, nCoord + float2(-waterTimer*0.015,-waterTimer*0.005)).rgb - 1.0;
	    nCoord = UV * (WAVE_SCALE * 0.1) + WIND_DIR * waterTimer * (WIND_SPEED*0.08)-(normal0.xy/normal0.zz)*WAVE_CHOPPYNESS;
	    float3 normal1 = 2.0 * shSample(normalMap, nCoord + float2(+waterTimer*0.020,+waterTimer*0.015)).rgb - 1.0;
     
     	nCoord = UV * (WAVE_SCALE * 0.25) + WIND_DIR * waterTimer * (WIND_SPEED*0.07)-(normal1.xy/normal1.zz)*WAVE_CHOPPYNESS;
	    float3 normal2 = 2.0 * shSample(normalMap, nCoord + float2(-waterTimer*0.04,-waterTimer*0.03)).rgb - 1.0;
	    nCoord = UV * (WAVE_SCALE * 0.5) + WIND_DIR * waterTimer * (WIND_SPEED*0.09)-(normal2.xy/normal2.z)*WAVE_CHOPPYNESS;
	    float3 normal3 = 2.0 * shSample(normalMap, nCoord + float2(+waterTimer*0.03,+waterTimer*0.04)).rgb - 1.0;
      
      	nCoord = UV * (WAVE_SCALE* 1.0) + WIND_DIR * waterTimer * (WIND_SPEED*0.4)-(normal3.xy/normal3.zz)*WAVE_CHOPPYNESS;
	    float3 normal4 = 2.0 * shSample(normalMap, nCoord + float2(-waterTimer*0.02,+waterTimer*0.1)).rgb - 1.0;  
        nCoord = UV * (WAVE_SCALE * 2.0) + WIND_DIR * waterTimer * (WIND_SPEED*0.7)-(normal4.xy/normal4.zz)*WAVE_CHOPPYNESS;
        float3 normal5 = 2.0 * shSample(normalMap, nCoord + float2(+waterTimer*0.1,-waterTimer*0.06)).rgb - 1.0;

	
	
	    float3 normal = (normal0 * BIG_WAVES_X + normal1 * BIG_WAVES_Y +
                                normal2 * MID_WAVES_X + normal3 * MID_WAVES_Y +
                                normal4 * SMALL_WAVES_X + normal5 * SMALL_WAVES_Y);

        float4 worldPosition = shMatrixMult(wMat, float4(position.xyz, 1));
        float2 relPos = (worldPosition.xy - rippleCenter.xy) / rippleAreaLength + 0.5;
        float3 normal_ripple = normalize(shSample(rippleNormalMap, relPos.xy).xyz * 2 - 1);

        //normal = normalize(normal + normal_ripple);
        normal = normalize(float3(normal.x * BUMP + normal_ripple.x, normal.y * BUMP + normal_ripple.y, normal.z));
        normal = float3(normal.x, normal.y, -normal.z);

	    // normal for sunlight scattering			        
		float3 lNormal = (normal0 * BIG_WAVES_X*0.5 + normal1 * BIG_WAVES_Y*0.5 +
                            normal2 * MID_WAVES_X*0.2 + normal3 * MID_WAVES_Y*0.2 +
                            normal4 * SMALL_WAVES_X*0.1 + normal5 * SMALL_WAVES_Y*0.1).xyz;
        lNormal = normalize(float3(lNormal.x * BUMP, lNormal.y * BUMP, lNormal.z));
        lNormal = float3(lNormal.x, lNormal.y, -lNormal.z);

        
        float3 lVec = normalize(sunPosition.xyz);
        float3 vVec = normalize(position.xyz - cameraPos.xyz);
        
        
        float isUnderwater = (cameraPos.z > 0) ? 0.0 : 1.0;
        
        // sunlight scattering
        float3 pNormal = float3(0,0,1);
    	float3 lR = reflect(lVec, lNormal);
        float3 llR = reflect(lVec, pNormal);
        
        float s = shSaturate(dot(lR, vVec)*2.0-1.2);
        float lightScatter = shSaturate(dot(-lVec,lNormal)*0.7+0.3) * s * SCATTER_AMOUNT * waterSunFade_sunHeight.x * shSaturate(1.0-exp(-waterSunFade_sunHeight.y));
        float3 scatterColour = shLerp(float3(SCATTER_COLOUR)*float3(1.0,0.4,0.0), SCATTER_COLOUR, shSaturate(1.0-exp(-waterSunFade_sunHeight.y*SUN_EXT)));

        // fresnel
        float ior = (cameraPos.z>0)?(1.333/1.0):(1.0/1.333); //air to water; water to air
        float fresnel = fresnel_dielectric(-vVec, normal, ior);
        
        fresnel = shSaturate(fresnel);
    
        // reflection
        float3 reflection = shSample(reflectionMap, screenCoords+(normal.xy*REFL_BUMP)).rgb;
		
		// refraction
        float3 R = reflect(vVec, normal);

#if REFRACTION
        float3 refraction = shSample(refractionMap, (screenCoords-(normal.xy*REFR_BUMP))*1.0).rgb;
        
         // brighten up the refraction underwater
        refraction = (cameraPos.z < 0) ? shSaturate(refraction * 1.5) : refraction;
#endif

		// specular
        float specular = pow(max(dot(R, lVec), 0.0),SPEC_HARDNESS);

#if REFRACTION
        shOutputColour(0).xyz = shLerp(  shLerp(refraction, scatterColour, lightScatter), reflection, fresnel) + specular * sunSpecular.xyz;
#else
        shOutputColour(0).xyz = shLerp(reflection, float3(0.18039, 0.23137, 0.25490), (1.0-fresnel)*0.5) + specular * sunSpecular.xyz;
#endif
        // fog
        float fogValue = shSaturate((depthPassthrough - fogParams.y) * fogParams.w);
        shOutputColour(0).xyz = shLerp (shOutputColour(0).xyz, fogColor, fogValue);

#if REFRACTION
                shOutputColour(0).w = 1;
#else
        shOutputColour(0).w = shSaturate(fresnel*2 + specular);
#endif
    }

#endif


#endif
