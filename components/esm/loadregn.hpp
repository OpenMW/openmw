#ifndef OPENMW_ESM_REGN_H
#define OPENMW_ESM_REGN_H

#include <string>
#include <vector>

#include "esmcommon.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Region data
 */

struct Region
{
    static unsigned int sRecordId;

#pragma pack(push)
#pragma pack(1)
    struct WEATstruct
    {
        // These are probabilities that add up to 100
        unsigned char mClear, mCloudy, mFoggy, mOvercast, mRain, mThunder, mAsh, mBlight,
        // Unknown weather, probably snow and something. Only
        // present in file version 1.3.
        // the engine uses mA as "snow" and mB as "blizard"
                mA, mB;
    }; // 10 bytes

    // Reference to a sound that is played randomly in this region
    struct SoundRef
    {
        NAME32 mSound;
        unsigned char mChance;
    }; // 33 bytes
#pragma pack(pop)

    WEATstruct mData;
    int mMapColor; // RGBA

    // sleepList refers to a eveled list of creatures you can meet if
    // you sleep outside in this region.
    std::string mId, mName, mSleepList;

    std::vector<SoundRef> mSoundList;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID/index).
};
}
#endif
