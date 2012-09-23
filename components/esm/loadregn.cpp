#include "loadregn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Region::load(ESMReader &esm)
{
    mName = esm.getHNString("FNAM");

    if (esm.getVer() == VER_12)
        esm.getHNExact(&mData, sizeof(mData) - 2, "WEAT");
    else if (esm.getVer() == VER_13)
        esm.getHNExact(&mData, sizeof(mData), "WEAT");
    else
        esm.fail("Don't know what to do in this version");

    mSleepList = esm.getHNOString("BNAM");

    esm.getHNT(mMapColor, "CNAM");

    while (esm.hasMoreSubs())
    {
        SoundRef sr;
        esm.getHNT(sr, "SNAM", 33);
        mSoundList.push_back(sr);
    }
}
void Region::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", mName);
    
    if (esm.getVersion() == VER_12)
        esm.writeHNT("WEAT", mData, sizeof(mData) - 2);
    else
        esm.writeHNT("WEAT", mData);
    
    esm.writeHNOCString("BNAM", mSleepList);
    
    esm.writeHNT("CNAM", mMapColor);
    for (std::vector<SoundRef>::iterator it = mSoundList.begin(); it != mSoundList.end(); ++it)
    {
        esm.writeHNT<SoundRef>("SNAM", *it);
    }
}

}
