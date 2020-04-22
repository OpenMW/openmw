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



}
