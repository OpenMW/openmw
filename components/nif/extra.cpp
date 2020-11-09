#include "extra.hpp"

namespace Nif
{

void NiStringExtraData::read(NIFStream *nif)
{
    Extra::read(nif);
    string = nif->getString();
}

void NiTextKeyExtraData::read(NIFStream *nif)
{
    Extra::read(nif);

    int keynum = nif->getInt();
    list.resize(keynum);
    for(int i=0; i<keynum; i++)
    {
        list[i].time = nif->getFloat();
        list[i].text = nif->getString();
    }
}

void NiVertWeightsExtraData::read(NIFStream *nif)
{
    Extra::read(nif);

    nif->skip(nif->getUShort() * sizeof(float)); // vertex weights I guess
}

void NiIntegerExtraData::read(NIFStream *nif)
{
    Extra::read(nif);

    data = nif->getUInt();
}

void NiIntegersExtraData::read(NIFStream *nif)
{
    Extra::read(nif);

    unsigned int num = nif->getUInt();
    if (num)
        nif->getUInts(data, num);
}

void NiBinaryExtraData::read(NIFStream *nif)
{
    Extra::read(nif);
    unsigned int size = nif->getUInt();
    if (size)
        nif->getChars(data, size);
}

void NiBooleanExtraData::read(NIFStream *nif)
{
    Extra::read(nif);
    data = nif->getBoolean();
}

void NiVectorExtraData::read(NIFStream *nif)
{
    Extra::read(nif);
    data = nif->getVector4();
}

void NiFloatExtraData::read(NIFStream *nif)
{
    Extra::read(nif);

    data = nif->getFloat();
}

void NiFloatsExtraData::read(NIFStream *nif)
{
    Extra::read(nif);
    unsigned int num = nif->getUInt();
    if (num)
        nif->getFloats(data, num);
}

void BSBound::read(NIFStream *nif)
{
    Extra::read(nif);
    center = nif->getVector3();
    halfExtents = nif->getVector3();
}

}
