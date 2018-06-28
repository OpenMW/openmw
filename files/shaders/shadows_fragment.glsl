#define SHADOWS @shadows_enabled

#if SHADOWS
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        uniform sampler2DShadow shadowTexture@shadow_texture_unit_index;
        varying vec4 shadowSpaceCoords@shadow_texture_unit_index;
    @endforeach
#endif // SHADOWS

float unshadowedLightRatio()
{
    float shadowing = 1.0;
#if SHADOWS
    #if @shadowMapsOverlap
        bool doneShadows = false;
        @foreach shadow_texture_unit_index @shadow_texture_unit_list
            if (!doneShadows)
            {
                vec2 shadowXY = shadowSpaceCoords@shadow_texture_unit_index.xy / shadowSpaceCoords@shadow_texture_unit_index.w;
                if (all(lessThan(shadowXY, vec2(1.0, 1.0))) && all(greaterThan(shadowXY, vec2(0.0, 0.0))))
                {
                    shadowing = min(shadow2DProj(shadowTexture@shadow_texture_unit_index, shadowSpaceCoords@shadow_texture_unit_index).r, shadowing);

                    if (all(lessThan(shadowXY, vec2(0.95, 0.95))) && all(greaterThan(shadowXY, vec2(0.05, 0.05))))
                        doneShadows = true;
                }
            }
        @endforeach
    #else
        @foreach shadow_texture_unit_index @shadow_texture_unit_list
            shadowing = min(shadow2DProj(shadowTexture@shadow_texture_unit_index, shadowSpaceCoords@shadow_texture_unit_index).r, shadowing);
        @endforeach
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
            vec2 shadowXY = shadowSpaceCoords@shadow_texture_unit_index.xy / shadowSpaceCoords@shadow_texture_unit_index.w;
            if (all(lessThan(shadowXY, vec2(1.0, 1.0))) && all(greaterThan(shadowXY, vec2(0.0, 0.0))))
            {
                colourIndex = mod(@shadow_texture_unit_index.0, 3.0);
		        if (colourIndex < 1.0)
			        gl_FragData[0].x += 0.1;
		        else if (colourIndex < 2.0)
			        gl_FragData[0].y += 0.1;
		        else
			        gl_FragData[0].z += 0.1;

                if (all(lessThan(shadowXY, vec2(0.95, 0.95))) && all(greaterThan(shadowXY, vec2(0.05, 0.05))))
                    doneOverlay = true;
            }
        }
    @endforeach
#endif // SHADOWS
}