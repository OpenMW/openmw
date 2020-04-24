#include "property.hpp"

#include "data.hpp"
#include "controlled.hpp"

namespace Nif
{

void Property::read(NIFStream *nif)
{
    Named::read(nif);
    flags = nif->getUShort();
}

void NiTexturingProperty::Texture::read(NIFStream *nif)
{
    inUse = nif->getBoolean();
    if(!inUse) return;

    texture.read(nif);
    clamp = nif->getUInt();
    nif->skip(4); // Filter mode. Ignoring because global filtering settings are more sensible
    uvSet = nif->getUInt();

    // Two PS2-specific shorts.
    nif->skip(4);
    nif->skip(2); // Unknown short
}

void NiTexturingProperty::Texture::post(NIFFile *nif)
{
    texture.post(nif);
}

void NiTexturingProperty::read(NIFStream *nif)
{
    Property::read(nif);
    apply = nif->getUInt();

    unsigned int numTextures = nif->getUInt();

    if (!numTextures)
        return;

    textures.resize(numTextures);
    for (unsigned int i = 0; i < numTextures; i++)
    {
        textures[i].read(nif);
        if (i == 5 && textures[5].inUse) // Bump map settings
        {
            envMapLumaBias = nif->getVector2();
            bumpMapMatrix = nif->getVector4();
        }
    }
}

void NiTexturingProperty::post(NIFFile *nif)
{
    Property::post(nif);
    for(int i = 0;i < 7;i++)
        textures[i].post(nif);
}

void NiFogProperty::read(NIFStream *nif)
{
    Property::read(nif);

    mFogDepth = nif->getFloat();
    mColour = nif->getVector3();
}

void S_MaterialProperty::read(NIFStream *nif)
{
    ambient = nif->getVector3();
    diffuse = nif->getVector3();
    specular = nif->getVector3();
    emissive = nif->getVector3();
    glossiness = nif->getFloat();
    alpha = nif->getFloat();
}

void S_VertexColorProperty::read(NIFStream *nif)
{
    vertmode = nif->getInt();
    lightmode = nif->getInt();
}

void S_AlphaProperty::read(NIFStream *nif)
{
    threshold = nif->getChar();
}

void S_StencilProperty::read(NIFStream *nif)
{
    enabled = nif->getChar();
    compareFunc = nif->getInt();
    stencilRef = nif->getUInt();
    stencilMask = nif->getUInt();
    failAction = nif->getInt();
    zFailAction = nif->getInt();
    zPassAction = nif->getInt();
    drawMode = nif->getInt();
}



}
