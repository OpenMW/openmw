#ifndef LIB_MATERIAL_PROPERTIES_H
#define LIB_MATERIAL_PROPERTIES_H

#include "lib/material/colormodes.glsl"

vec4 getEmissionColor(Material material, vec4 passColor)
{
    if (material.vertexColorMode == ColorMode_Emission)
        return passColor;
    return material.emission;
}

vec4 getAmbientColor(Material material, vec4 passColor)
{
    if (material.vertexColorMode == ColorMode_AmbientAndDiffuse || material.vertexColorMode == ColorMode_Ambient)
        return passColor;
    return material.ambient;
}

vec4 getDiffuseColor(Material material, vec4 passColor)
{
    if (material.vertexColorMode == ColorMode_AmbientAndDiffuse || material.vertexColorMode == ColorMode_Diffuse)
        return passColor;
    return material.diffuse;
}

vec4 getSpecularColor(Material material, vec4 passColor)
{
    if (material.vertexColorMode == ColorMode_Specular)
        return passColor;
    return material.specular;
}

bool skipLighting(Material material, vec4 passColor)
{
    return getAmbientColor(material, passColor).xyz == vec3(0.0) && getDiffuseColor(material, passColor).xyz == vec3(0.0) && getSpecularColor(material, passColor).xyz == vec3(0.0);
}

#endif
