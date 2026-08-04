#ifndef LIB_MATERIAL_MATERIAL
#define LIB_MATERIAL_MATERIAL

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    float shininess;
    float emissiveMult;
    float specStrength;
    int colorMode;
};

#endif
