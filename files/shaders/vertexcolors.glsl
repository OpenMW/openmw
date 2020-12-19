centroid varying vec4 passColor;

uniform int colorMode;

const int ColorMode_None = 0;
const int ColorMode_Emission = 1;
const int ColorMode_AmbientAndDiffuse = 2;
const int ColorMode_Ambient = 3;
const int ColorMode_Diffuse = 4;
const int ColorMode_Specular = 5;

vec4 getEmissionColor()
{
    if (colorMode == ColorMode_Emission)
        return passColor;
    return gl_FrontMaterial.emission;
}

vec4 getAmbientColor()
{
    if (colorMode == ColorMode_AmbientAndDiffuse || colorMode == ColorMode_Ambient)
        return passColor;
    return gl_FrontMaterial.ambient;
}

vec4 getDiffuseColor()
{
    if (colorMode == ColorMode_AmbientAndDiffuse || colorMode == ColorMode_Diffuse)
        return passColor;
    return gl_FrontMaterial.diffuse;
}

vec4 getSpecularColor()
{
    if (colorMode == ColorMode_Specular)
        return passColor;
    return gl_FrontMaterial.specular;
}
