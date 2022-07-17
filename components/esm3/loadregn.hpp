#ifndef OPENMW_ESM_REGN_H
#define OPENMW_ESM_REGN_H

#include <string>
#include <vector>

#include "components/esm/defs.hpp"
#include "components/esm/esmcommon.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Region data
 */

struct Region
{
    constexpr static RecNameInts sRecordId = REC_REGN;

    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string_view getRecordType() { return "Region"; }

#pragma pack(push)
#pragma pack(1)
    struct WEATstruct
    {
        // These are probabilities that add up to 100
        unsigned char mClear, mCloudy, mFoggy, mOvercast, mRain, mThunder, mAsh, mBlight, mSnow, mBlizzard;
    }; // 10 bytes
#pragma pack(pop)

    // Reference to a sound that is played randomly in this region
    struct SoundRef
    {
        std::string   mSound;
        unsigned char mChance;
    };

    WEATstruct mData;
    int mMapColor; // RGBA

    unsigned int mRecordFlags;
    // sleepList refers to a leveled list of creatures you can meet if
    // you sleep outside in this region.
    std::string mId, mName, mSleepList;

    std::vector<SoundRef> mSoundList;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID/index).
};
}
#endif
