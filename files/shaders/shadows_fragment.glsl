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