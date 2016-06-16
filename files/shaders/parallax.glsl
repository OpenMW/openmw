#define PARALLAX_SCALE 0.04
#define PARALLAX_BIAS -0.02

vec2 getParallaxOffset(vec3 eyeDir, mat3 tbnTranspose, float height, float flipY)
{
    vec3 TSeyeDir = normalize(eyeDir * tbnTranspose);
    return vec2(TSeyeDir.x, TSeyeDir.y * flipY) * ( height * PARALLAX_SCALE + PARALLAX_BIAS );
}
