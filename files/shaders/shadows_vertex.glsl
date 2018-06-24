#define SHADOWS @shadows_enabled

#if SHADOWS
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        uniform int shadowTextureUnit@shadow_texture_unit_index;
        varying vec4 shadowSpaceCoords@shadow_texture_unit_index;
    @endforeach
#endif // SHADOWS

void setupShadowCoords(vec4 viewPos)
{
#if SHADOWS
    // This matrix has the opposite handedness to the others used here, so multiplication must have the vector to the left. Alternatively it could be transposed after construction, but that's extra work for the GPU just to make the code look a tiny bit cleaner.
    mat4 eyePlaneMat;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        eyePlaneMat = mat4(gl_EyePlaneS[shadowTextureUnit@shadow_texture_unit_index], gl_EyePlaneT[shadowTextureUnit@shadow_texture_unit_index], gl_EyePlaneR[shadowTextureUnit@shadow_texture_unit_index], gl_EyePlaneQ[shadowTextureUnit@shadow_texture_unit_index]);
        shadowSpaceCoords@shadow_texture_unit_index = viewPos * eyePlaneMat;
    @endforeach
#endif // SHADOWS
}