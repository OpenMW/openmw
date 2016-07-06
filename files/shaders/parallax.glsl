#define PARALLAX_SCALE 0.04
#define PARALLAX_BIAS -0.02

vec2 getParallaxOffset(vec3 eyeDir, mat3 tbn, float height)
{
    vec3 TSeyeDir = normalize((vec4(normalize(tbn * eyeDir),0)).xyz);
    return TSeyeDir.xy * ( height * PARALLAX_SCALE + PARALLAX_BIAS );
}
