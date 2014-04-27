#include "loadregn.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Region::sRecordId = REC_REGN;

void Region::load(ESMReader &esm)
{
    mName = esm.getHNOString("FNAM");

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
void Region::save(ESMWriter &esm) const
{
    esm.writeHNOCString("FNAM", mName);

    if (esm.getVersion() == VER_12)
        esm.writeHNT("WEAT", mData, sizeof(mData) - 2);
    else
        esm.writeHNT("WEAT", mData);

    esm.writeHNOCString("BNAM", mSleepList);

    esm.writeHNT("CNAM", mMapColor);
    for (std::vector<SoundRef>::const_iterator it = mSoundList.begin(); it != mSoundList.end(); ++it)
    {
        esm.writeHNT<SoundRef>("SNAM", *it);
    }
}

    void Region::blank()
    {
        mName.clear();

        mData.mClear = mData.mCloudy = mData.mFoggy = mData.mOvercast = mData.mRain =
            mData.mThunder = mData.mAsh, mData.mBlight = mData.mA = mData.mB = 0;

        mMapColor = 0;

        mName.clear();
        mSleepList.clear();
        mSoundList.clear();
    }
}
