#include "loadsoun.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Sound::sRecordId = REC_SOUN;

void Sound::load(ESMReader &esm)
{
    mSound = esm.getHNOString("FNAM");
    esm.getHNT(mData, "DATA", 3);
    /*
     cout << "vol=" << (int)data.volume
     << " min=" << (int)data.minRange
     << " max=" << (int)data.maxRange
     << endl;
     */
}
void Sound::save(ESMWriter &esm) const
{
    esm.writeHNOCString("FNAM", mSound);
    esm.writeHNT("DATA", mData, 3);
}

    void Sound::blank()
    {
        mSound.clear();

        mData.mVolume = 128;
        mData.mMinRange = 0;
        mData.mMaxRange = 255;
    }
}
