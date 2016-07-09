#include "effect.hpp"

#include "node.hpp"

namespace Nif
{

void NiLight::read(NIFStream *nif)
{
    NiDynamicEffect::read(nif);

    dimmer = nif->getFloat();
    ambient = nif->getVector3();
    diffuse = nif->getVector3();
    specular = nif->getVector3();
}

void NiTextureEffect::read(NIFStream *nif)
{
    NiDynamicEffect::read(nif);

    // Model Projection Matrix
    nif->skip(3 * 3 * sizeof(float));

    // Model Projection Transform
    nif->skip(3 * sizeof(float));

    // Texture Filtering
    nif->skip(4);

    clamp = nif->getUInt();

    textureType = (TextureType)nif->getUInt();

    coordGenType = (CoordGenType)nif->getUInt();

    texture.read(nif);

    /*
           byte = 0
           vector4 = [1,0,0,0]
           short = 0
           short = -75
           short = 0
        */
    nif->skip(23);
}

void NiTextureEffect::post(NIFFile *nif)
{
    NiDynamicEffect::post(nif);
    texture.post(nif);
}

void NiPointLight::read(NIFStream *nif)
{
    NiLight::read(nif);

    constantAttenuation = nif->getFloat();
    linearAttenuation = nif->getFloat();
    quadraticAttenuation = nif->getFloat();
}

void NiSpotLight::read(NIFStream *nif)
{
    NiPointLight::read(nif);

    cutoff = nif->getFloat();
    exponent = nif->getFloat();
}

}
