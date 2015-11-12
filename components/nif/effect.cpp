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

    /*
           3 x Vector4 = [1,0,0,0]
           int = 2
           int = 0 or 3
           int = 2
           int = 2
        */
    nif->skip(16*4);

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
