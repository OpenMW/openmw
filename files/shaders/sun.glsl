#if !@ffpLighting
struct Sunlight
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 direction;
};

layout(std140) uniform SunlightBuffer
{
    Sunlight Sun;
};
#endif