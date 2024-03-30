#define SHADOWS @shadows_enabled

#if SHADOWS
    uniform float maximumShadowMapDistance;
    uniform float shadowFadeStart;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        uniform sampler2D shadowTexture@shadow_texture_unit_index;
        varying vec4 shadowSpaceCoords@shadow_texture_unit_index;

#if @perspectiveShadowMaps
        varying vec4 shadowRegionCoords@shadow_texture_unit_index;
#endif
    @endforeach
#endif // SHADOWS

float lerp(float a, float b, float w)
{
    return a + w*(b-a);
}

float PCFBase(sampler2D shadowTexture, vec3 shadowCoordsProj)
{
    float Offset = 1.0/@shadowMapSize.0;
    float res1, res2, res3, res4;
    float bias = 0.999999;

    vec2 offset = vec2(-0.5 * Offset, 0.5 * Offset);
    float shadowDistance1 = texture2D(shadowTexture, shadowCoordsProj.xy + offset).r;
    if (shadowDistance1 >= shadowCoordsProj.z || shadowDistance1 > bias)
        res1 = 1.0;

    offset = vec2(0.5 * Offset, 0.5 * Offset);
    float shadowDistance2 = texture2D(shadowTexture, shadowCoordsProj.xy + offset).r;
    if (shadowDistance2 >= shadowCoordsProj.z || shadowDistance2 > bias)
        res2 = 1.0;

    offset = vec2(-0.5 * Offset, -0.5 * Offset);
    float shadowDistance3 = texture2D(shadowTexture, shadowCoordsProj.xy + offset).r;
    if (shadowDistance3 >= shadowCoordsProj.z || shadowDistance3 > bias)
        res3 = 1.0;

    offset = vec2(0.5 * Offset, -0.5 * Offset);
    float shadowDistance4 = texture2D(shadowTexture, shadowCoordsProj.xy + offset).r;
    if (shadowDistance4 >= shadowCoordsProj.z || shadowDistance4 > bias)
        res4 = 1.0;

    float weightx = fract(@shadowMapSize.0 * (shadowCoordsProj.x + offset.x));
    float weighty = fract(@shadowMapSize.0 * (shadowCoordsProj.y + offset.y));

    return lerp( lerp(res3, res4, weightx), lerp(res1, res2, weightx), weighty );
}

float calcShadowing(sampler2D shadowTexture, vec4 LightSpacePos)
{
    vec3 shadowCoordsProj = LightSpacePos.xyz / LightSpacePos.w;
    float Offset = 1.0/@shadowMapSize.0;
    float shadowing = 1.0;

#if @PCFSamples == 0
    float shadowDistance = texture2D(shadowTexture, shadowCoordsProj.xy).r;
    if (shadowDistance <= shadowCoordsProj.z && shadowDistance < 0.999999)
        shadowing =  0.0;
#else
    float numPixels = @PCFSamples.0;
    float mult = (numPixels -1.0) / 2.0;
    shadowing = 0.0;

    for (float y = -1.0 * mult; y <= mult; y+=1.0) {
        for (float x = -1.0 * mult; x <= mult; x+=1.0) {
            vec3 Offsets = vec3(float(x) * Offset, float(y) * Offset, -0.001);
                shadowing += PCFBase(shadowTexture, shadowCoordsProj + Offsets);
        }
    }
    shadowing = shadowing / (numPixels * numPixels);
#endif

    return  shadowing;
}

float unshadowedLightRatio(float distance)
{
    float shadowing = 1.0;
#if SHADOWS
#if @limitShadowMapDistance
    float fade = clamp((distance - shadowFadeStart) / (maximumShadowMapDistance - shadowFadeStart), 0.0, 1.0);
    if (fade == 1.0)
        return shadowing;
#endif
    #if @shadowMapsOverlap
        bool doneShadows = false;
        @foreach shadow_texture_unit_index @shadow_texture_unit_list
            if (!doneShadows)
            {
                vec3 shadowXYZ = shadowSpaceCoords@shadow_texture_unit_index.xyz / shadowSpaceCoords@shadow_texture_unit_index.w;
#if @perspectiveShadowMaps
                vec3 shadowRegionXYZ = shadowRegionCoords@shadow_texture_unit_index.xyz / shadowRegionCoords@shadow_texture_unit_index.w;
#endif
                if (all(lessThan(shadowXYZ.xy, vec2(1.0, 1.0))) && all(greaterThan(shadowXYZ.xy, vec2(0.0, 0.0))))
                {
                    shadowing = calcShadowing(shadowTexture@shadow_texture_unit_index, shadowSpaceCoords@shadow_texture_unit_index);

                    doneShadows = all(lessThan(shadowXYZ, vec3(0.95, 0.95, 1.0))) && all(greaterThan(shadowXYZ, vec3(0.05, 0.05, 0.0)));
#if @perspectiveShadowMaps
                    doneShadows = doneShadows && all(lessThan(shadowRegionXYZ, vec3(1.0, 1.0, 1.0))) && all(greaterThan(shadowRegionXYZ.xy, vec2(-1.0, -1.0)));
#endif
                }
            }
        @endforeach
    #else
        @foreach shadow_texture_unit_index @shadow_texture_unit_list
            shadowing = calcShadowing(shadowTexture@shadow_texture_unit_index, shadowSpaceCoords@shadow_texture_unit_index);
        @endforeach
    #endif
#if @limitShadowMapDistance
    shadowing = mix(shadowing, 1.0, fade);
#endif
#endif // SHADOWS
    return shadowing;
}

void applyShadowDebugOverlay()
{
#if SHADOWS && @useShadowDebugOverlay
    bool doneOverlay = false;
    float colourIndex = 0.0;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        if (!doneOverlay)
        {
            vec3 shadowXYZ = shadowSpaceCoords@shadow_texture_unit_index.xyz / shadowSpaceCoords@shadow_texture_unit_index.w;
#if @perspectiveShadowMaps
            vec3 shadowRegionXYZ = shadowRegionCoords@shadow_texture_unit_index.xyz / shadowRegionCoords@shadow_texture_unit_index.w;
#endif
            if (all(lessThan(shadowXYZ.xy, vec2(1.0, 1.0))) && all(greaterThan(shadowXYZ.xy, vec2(0.0, 0.0))))
            {
                colourIndex = mod(@shadow_texture_unit_index.0, 3.0);
                if (colourIndex < 1.0)
                    gl_FragData[0].x += 0.1;
                else if (colourIndex < 2.0)
                    gl_FragData[0].y += 0.1;
                else
                    gl_FragData[0].z += 0.1;

                doneOverlay = all(lessThan(shadowXYZ, vec3(0.95, 0.95, 1.0))) && all(greaterThan(shadowXYZ, vec3(0.05, 0.05, 0.0)));
#if @perspectiveShadowMaps
                doneOverlay = doneOverlay && all(lessThan(shadowRegionXYZ.xyz, vec3(1.0, 1.0, 1.0))) && all(greaterThan(shadowRegionXYZ.xy, vec2(-1.0, -1.0)));
#endif
            }
        }
    @endforeach
#endif // SHADOWS
}
