#include "extra.hpp"

namespace Nif
{

void NiStringExtraData::read(NIFStream *nif)
{
    Extra::read(nif);

    nif->getInt(); // size of string + 4. Really useful...
    string = nif->getString();
}

void NiTextKeyExtraData::read(NIFStream *nif)
{
    Extra::read(nif);

    nif->getInt(); // 0

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

    // We should have s*4+2 == i, for some reason. Might simply be the
    // size of the rest of the record, unhelpful as that may be.
    /*int i =*/ nif->getInt();
    int s = nif->getUShort();

    nif->skip(s * sizeof(float)); // vertex weights I guess
}



}
