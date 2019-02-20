#define SHADOWS @shadows_enabled

#if SHADOWS
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        uniform int shadowTextureUnit@shadow_texture_unit_index;
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
    // This matrix has the opposite handedness to the others used here, so multiplication must have the vector to the left. Alternatively it could be transposed after construction, but that's extra work for the GPU just to make the code look a tiny bit cleaner.
    mat4 eyePlaneMat;
    vec4 shadowOffset;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        eyePlaneMat = mat4(gl_EyePlaneS[shadowTextureUnit@shadow_texture_unit_index], gl_EyePlaneT[shadowTextureUnit@shadow_texture_unit_index], gl_EyePlaneR[shadowTextureUnit@shadow_texture_unit_index], gl_EyePlaneQ[shadowTextureUnit@shadow_texture_unit_index]);

#if @perspectiveShadowMaps
        shadowRegionCoords@shadow_texture_unit_index = validRegionMatrix@shadow_texture_unit_index * viewPos;
#endif
        
#if @disableNormalOffsetShadows
        shadowSpaceCoords@shadow_texture_unit_index = viewPos * eyePlaneMat;
#else
        shadowOffset = vec4(viewNormal * @shadowNormalOffset, 0.0);

        if (onlyNormalOffsetUV)
        {
            shadowSpaceCoords@shadow_texture_unit_index = viewPos * eyePlaneMat;

            vec4 lightSpaceXY = viewPos + shadowOffset;
            lightSpaceXY = lightSpaceXY * eyePlaneMat;

            shadowSpaceCoords@shadow_texture_unit_index.xy = lightSpaceXY.xy;
        }
        else
        {
            vec4 offsetViewPosition = viewPos + shadowOffset;
            shadowSpaceCoords@shadow_texture_unit_index = offsetViewPosition * eyePlaneMat;
        }
#endif
    @endforeach
#endif // SHADOWS
}