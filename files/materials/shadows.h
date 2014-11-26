
#define FIXED_BIAS 0.0003

float depthShadowPCF (shTexture2D shadowMap, float4 shadowMapPos, float2 offset)
{
    shadowMapPos /= shadowMapPos.w;
    float3 o = float3(offset.xy, -offset.x) * 0.3;
    //float3 o = float3(0,0,0);
    float c =   (shadowMapPos.z <= FIXED_BIAS + shSample(shadowMap, shadowMapPos.xy - o.xy).r) ? 1.0 : 0.0; // top left
    c +=        (shadowMapPos.z <= FIXED_BIAS + shSample(shadowMap, shadowMapPos.xy + o.xy).r) ? 1.0 : 0.0; // bottom right
    c +=        (shadowMapPos.z <= FIXED_BIAS + shSample(shadowMap, shadowMapPos.xy + o.zy).r) ? 1.0 : 0.0; // bottom left
    c +=        (shadowMapPos.z <= FIXED_BIAS + shSample(shadowMap, shadowMapPos.xy - o.zy).r) ? 1.0 : 0.0; // top right
    return c / 4.0;
}



float pssmDepthShadow (


    float4 lightSpacePos0,
    float2 invShadowmapSize0,
    shTexture2D shadowMap0,
    
    float4 lightSpacePos1,
    float2 invShadowmapSize1,
    shTexture2D shadowMap1,
    
    float4 lightSpacePos2,
    float2 invShadowmapSize2,
    shTexture2D shadowMap2,
    
    float depth,
    float3 pssmSplitPoints)

{
    float shadow;
    
    float pcf1 = depthShadowPCF(shadowMap0, lightSpacePos0, invShadowmapSize0);
    float pcf2 = depthShadowPCF(shadowMap1, lightSpacePos1, invShadowmapSize1);
    float pcf3 = depthShadowPCF(shadowMap2, lightSpacePos2, invShadowmapSize2);
    
    if (depth < pssmSplitPoints.x)
        shadow = pcf1;
    else if (depth < pssmSplitPoints.y)
        shadow = pcf2;
    else
        shadow = pcf3;

    return shadow;
}
