#include "extra.hpp"

namespace Nif
{

void NiExtraData::read(NIFStream *nif)
{
    Extra::read(nif);
    if (recordSize)
        nif->getChars(data, recordSize);
}

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

void BSFurnitureMarker::LegacyFurniturePosition::read(NIFStream *nif)
{
    offset = nif->getVector3();
    orientation = nif->getUShort();
    positionRef = nif->getChar();
    nif->skip(1); // Position ref 2
}

void BSFurnitureMarker::FurniturePosition::read(NIFStream *nif)
{
    offset = nif->getVector3();
    heading = nif->getFloat();
    type = nif->getUShort();
    entryPoint = nif->getUShort();
}

void BSFurnitureMarker::read(NIFStream *nif)
{
    Extra::read(nif);
    unsigned int num = nif->getUInt();
    if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
    {
        legacyMarkers.resize(num);
        for (auto& marker : legacyMarkers)
            marker.read(nif);
    }
    else
    {
        markers.resize(num);
        for (auto& marker : markers)
            marker.read(nif);
    }
}

}
