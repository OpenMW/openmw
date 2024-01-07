#ifndef LIB_MATERIAL_PARALLAX
#define LIB_MATERIAL_PARALLAX

#define PARALLAX_SCALE 0.04
#define PARALLAX_BIAS -0.02

vec2 getParallaxOffset(vec3 eyeDir, float height, float flipY)
{
    return vec2(eyeDir.x, eyeDir.y * flipY) * ( height * PARALLAX_SCALE + PARALLAX_BIAS );
}

#endif
