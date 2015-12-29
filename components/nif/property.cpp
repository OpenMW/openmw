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
    inUse = !!nif->getInt();
    if(!inUse) return;

    texture.read(nif);
    clamp = nif->getInt();
    filter = nif->getInt();
    uvSet = nif->getInt();

    // I have no idea, but I think these are actually two
    // PS2-specific shorts (ps2L and ps2K), followed by an unknown
    // short.
    nif->skip(6);
}

void NiTexturingProperty::Texture::post(NIFFile *nif)
{
    texture.post(nif);
}

void NiTexturingProperty::read(NIFStream *nif)
{
    Property::read(nif);
    apply = nif->getInt();

    // Unknown, always 7. Probably the number of textures to read
    // below
    nif->getInt();

    textures[0].read(nif); // Base
    textures[1].read(nif); // Dark
    textures[2].read(nif); // Detail
    textures[3].read(nif); // Gloss (never present)
    textures[4].read(nif); // Glow
    textures[5].read(nif); // Bump map
    if(textures[5].inUse)
    {
        // Ignore these at the moment
        /*float lumaScale =*/ nif->getFloat();
        /*float lumaOffset =*/ nif->getFloat();
        /*const Vector4 *lumaMatrix =*/ nif->getVector4();
    }
    textures[6].read(nif); // Decal
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
