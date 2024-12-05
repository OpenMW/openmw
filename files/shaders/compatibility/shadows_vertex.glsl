#define SHADOWS @shadows_enabled

#if SHADOWS
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        uniform mat4 shadowSpaceMatrix@shadow_texture_unit_index;
        varying vec4 shadowSpaceCoords@shadow_texture_unit_index;

#if @perspectiveShadowMaps
        uniform mat4 validRegionMatrix@shadow_texture_unit_index;
        varying vec4 shadowRegionCoords@shadow_texture_unit_index;
#endif
    @endforeach

    // Enabling this may reduce peter panning. Probably unnecessary.
    const bool onlyNormalOffsetUV = false;
#endif // SHADOWS

void setupShadowCoords(vec4 viewPos, vec3 viewNormal)
{
#if SHADOWS
    vec4 shadowOffset;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
#if @perspectiveShadowMaps
        shadowRegionCoords@shadow_texture_unit_index = validRegionMatrix@shadow_texture_unit_index * viewPos;
#endif

#if @disableNormalOffsetShadows
        shadowSpaceCoords@shadow_texture_unit_index = shadowSpaceMatrix@shadow_texture_unit_index * viewPos;
#else
        shadowOffset = vec4(viewNormal * @shadowNormalOffset, 0.0);

        if (onlyNormalOffsetUV)
        {
            vec4 lightSpaceXY = viewPos + shadowOffset;
            lightSpaceXY = shadowSpaceMatrix@shadow_texture_unit_index * lightSpaceXY;

            shadowSpaceCoords@shadow_texture_unit_index.xy = lightSpaceXY.xy;
        }
        else
        {
            vec4 offsetViewPosition = viewPos + shadowOffset;
            shadowSpaceCoords@shadow_texture_unit_index = shadowSpaceMatrix@shadow_texture_unit_index * offsetViewPosition;
        }
#endif
    @endforeach
#endif // SHADOWS
}
