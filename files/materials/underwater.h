#define UNDERWATER_COLOUR float3(0.18039, 0.23137, 0.25490)

#define VISIBILITY 1000.0      // how far you can look through water

#define BIG_WAVES_X 0.3                     // strength of big waves
#define BIG_WAVES_Y 0.3   
                  
#define MID_WAVES_X 0.3                     // strength of middle sized waves
#define MID_WAVES_Y 0.15        

#define SMALL_WAVES_X 0.15                  // strength of small waves
#define SMALL_WAVES_Y 0.1      

#define WAVE_CHOPPYNESS 0.15                // wave choppyness
#define WAVE_SCALE 0.01                      // overall wave scale

#define ABBERATION 0.001                    // chromatic abberation amount

#define SUN_EXT float3(0.45, 0.55, 0.68)    //sunlight extinction

float3 intercept(float3 lineP,
               float3 lineN,
               float3 planeN,
               float  planeD)
{
  
    float distance = (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
    return lineP + lineN * distance;
}

float3 perturb1(shTexture2D tex, float2 coords, float bend, float2 windDir, float windSpeed, float timer)
{
    float2 nCoord = float2(0,0);
    bend *= WAVE_CHOPPYNESS;
  	nCoord = coords * (WAVE_SCALE * 0.05) + windDir * timer * (windSpeed*0.04);
    float3 normal0 = 2.0 * shSample(tex, nCoord + float2(-timer*0.015,-timer*0.05)).rgb - 1.0;
    nCoord = coords * (WAVE_SCALE * 0.1) + windDir * timer * (windSpeed*0.08)-normal0.xy*bend;
    float3 normal1 = 2.0 * shSample(tex, nCoord + float2(+timer*0.020,+timer*0.015)).rgb - 1.0;
 
 	nCoord = coords * (WAVE_SCALE * 0.25) + windDir * timer * (windSpeed*0.07)-normal1.xy*bend;
    float3 normal2 = 2.0 * shSample(tex, nCoord + float2(-timer*0.04,-timer*0.03)).rgb - 1.0;
    nCoord = coords * (WAVE_SCALE * 0.5) + windDir * timer * (windSpeed*0.09)-normal2.xy*bend;
    float3 normal3 = 2.0 * shSample(tex, nCoord + float2(+timer*0.03,+timer*0.04)).rgb - 1.0;
  
  	nCoord = coords * (WAVE_SCALE* 1.0) + windDir * timer * (windSpeed*0.4)-normal3.xy*bend;
    float3 normal4 = 2.0 * shSample(tex, nCoord + float2(-timer*0.2,+timer*0.1)).rgb - 1.0;  
    nCoord = coords * (WAVE_SCALE * 2.0) + windDir * timer * (windSpeed*0.7)-normal4.xy*bend;
    float3 normal5 = 2.0 * shSample(tex, nCoord + float2(+timer*0.1,-timer*0.06)).rgb - 1.0;


    float3 normal = normalize(normal0 * BIG_WAVES_X + normal1 * BIG_WAVES_Y +
                            normal2 * MID_WAVES_X + normal3 * MID_WAVES_Y +
					        normal4 * SMALL_WAVES_X + normal5 * SMALL_WAVES_Y);
    return normal;
}

float3 perturb(shTexture2D tex, float2 coords, float bend, float2 windDir, float windSpeed, float timer)
{
    bend *= WAVE_CHOPPYNESS;
    float3 col = float3(0,0,0);
    float2 nCoord = float2(0,0); //normal coords

    nCoord = coords * (WAVE_SCALE * 0.025) + windDir * timer * (windSpeed*0.03);
    col += shSample(tex,nCoord + float2(-timer*0.005,-timer*0.01)).rgb*0.20;
    nCoord = coords * (WAVE_SCALE * 0.1) + windDir * timer * (windSpeed*0.05)-(col.xy/col.zz)*bend;
    col += shSample(tex,nCoord + float2(+timer*0.01,+timer*0.005)).rgb*0.20;

    nCoord = coords * (WAVE_SCALE * 0.2) + windDir * timer * (windSpeed*0.1)-(col.xy/col.zz)*bend;
    col += shSample(tex,nCoord + float2(-timer*0.02,-timer*0.03)).rgb*0.20;
    nCoord = coords * (WAVE_SCALE * 0.5) + windDir * timer * (windSpeed*0.2)-(col.xy/col.zz)*bend;
    col += shSample(tex,nCoord + float2(+timer*0.03,+timer*0.02)).rgb*0.15;

    nCoord = coords * (WAVE_SCALE* 0.8) + windDir * timer * (windSpeed*1.0)-(col.xy/col.zz)*bend;
    col += shSample(tex, nCoord + float2(-timer*0.06,+timer*0.08)).rgb*0.15;
    nCoord = coords * (WAVE_SCALE * 1.0) + windDir * timer * (windSpeed*1.3)-(col.xy/col.zz)*bend;
    col += shSample(tex,nCoord + float2(+timer*0.08,-timer*0.06)).rgb*0.10;

    return col;
}


float3 getCaustics (shTexture2D causticMap, float3 worldPos, float3 waterEyePos, float3 worldNormal, float3 lightDirectionWS0, float waterLevel, float waterTimer, float3 windDir_windSpeed)
{
    float waterDepth = shSaturate((waterEyePos.z - worldPos.z) / 50.0);

    float3 causticPos = intercept(worldPos.xyz, lightDirectionWS0.xyz, float3(0,0,1), waterLevel);
    
    ///\ todo clean this up 
    float causticdepth = length(causticPos-worldPos.xyz);
    causticdepth = 1.0-shSaturate(causticdepth / VISIBILITY);
    causticdepth = shSaturate(causticdepth);
    
    // NOTE: the original shader calculated a tangent space basis here, 
    // but using only the world normal is cheaper and i couldn't see a visual difference
    // also, if this effect gets moved to screen-space some day, it's unlikely to have tangent information
    float3 causticNorm = worldNormal.xyz * perturb(causticMap, causticPos.xy, causticdepth, windDir_windSpeed.xy, windDir_windSpeed.z, waterTimer).xyz * 2 - 1;
    causticNorm = float3(causticNorm.x, causticNorm.y, -causticNorm.z);

    //float fresnel = pow(clamp(dot(LV,causticnorm),0.0,1.0),2.0); 
    
    float NdotL = max(dot(worldNormal.xyz, lightDirectionWS0.xyz),0.0);

    float causticR = 1.0-perturb(causticMap, causticPos.xy, causticdepth, windDir_windSpeed.xy, windDir_windSpeed.z, waterTimer).z;

    /// \todo sunFade
    
   // float3 caustics = clamp(pow(float3(causticR)*5.5,float3(5.5*causticdepth)),0.0,1.0)*NdotL*sunFade*causticdepth;
    float3 caustics = clamp(pow(float3(causticR,causticR,causticR)*5.5,float3(5.5*causticdepth,5.5*causticdepth,5.5*causticdepth)),0.0,1.0)*NdotL*causticdepth;
    float causticG = 1.0-perturb(causticMap,causticPos.xy+(1.0-causticdepth)*ABBERATION, causticdepth, windDir_windSpeed.xy, windDir_windSpeed.z, waterTimer).z;
    float causticB = 1.0-perturb(causticMap,causticPos.xy+(1.0-causticdepth)*ABBERATION*2.0, causticdepth, windDir_windSpeed.xy, windDir_windSpeed.z, waterTimer).z;
    //caustics = shSaturate(pow(float3(causticR,causticG,causticB)*5.5,float3(5.5*causticdepth)))*NdotL*sunFade*causticdepth;
                caustics = shSaturate(pow(float3(causticR,causticG,causticB)*5.5,float3(5.5*causticdepth,5.5*causticdepth,5.5*causticdepth)))*NdotL*causticdepth;

    caustics *= 3;
    
    // shore transition
    caustics = shLerp (float3(1,1,1), caustics, waterDepth);

    return caustics;
}
  
