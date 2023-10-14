#ifndef OPENMW_ESM_REGN_H
#define OPENMW_ESM_REGN_H

#include <array>
#include <string>
#include <vector>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

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

        struct WEATstruct
        {
            // These are probabilities that add up to 100
            // Clear, Cloudy, Foggy, Overcast, Rain, Thunder, Ash, Blight, Snow, Blizzard
            std::array<uint8_t, 10> mProbabilities;
        }; // 10 bytes

        // Reference to a sound that is played randomly in this region
        struct SoundRef
        {
            ESM::RefId mSound;
            uint8_t mChance;
        };

        WEATstruct mData;
        int32_t mMapColor; // RGBA

        uint32_t mRecordFlags;
        // sleepList refers to a leveled list of creatures you can meet if
        // you sleep outside in this region.
        RefId mId, mSleepList;
        std::string mName;

        std::vector<SoundRef> mSoundList;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).
    };
}
#endif
