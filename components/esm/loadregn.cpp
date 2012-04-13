#include "loadregn.hpp"

namespace ESM
{

void Region::load(ESMReader &esm)
{
    name = esm.getHNString("FNAM");

    if (esm.getVer() == VER_12)
        esm.getHNExact(&data, sizeof(data) - 2, "WEAT");
    else if (esm.getVer() == VER_13)
        esm.getHNExact(&data, sizeof(data), "WEAT");
    else
        esm.fail("Don't know what to do in this version");

    sleepList = esm.getHNOString("BNAM");

    esm.getHNT(mapColor, "CNAM");

    while (esm.hasMoreSubs())
    {
        SoundRef sr;
        esm.getHNT(sr, "SNAM", 33);
        soundList.push_back(sr);
    }
}
void Region::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", name);
    
    if (esm.getVersion() == VER_12)
        esm.writeHNT("WEAT", data, sizeof(data) - 2);
    else
        esm.writeHNT("WEAT", data);
    
    esm.writeHNOCString("BNAM", sleepList);
    
    esm.writeHNT("CNAM", mapColor);
    for (std::vector<SoundRef>::iterator it = soundList.begin(); it != soundList.end(); ++it)
    {
        esm.writeHNT<SoundRef>("SNAM", *it);
    }
}

}
