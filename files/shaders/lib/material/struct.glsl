#ifndef LIB_MATERIAL_STRUCT
#define LIB_MATERIAL_STRUCT

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    float shininess;
    float emissiveMult;
    float specStrength;
    int vertexColorMode;
};

#endif
