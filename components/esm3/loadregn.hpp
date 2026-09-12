#ifndef OPENMW_ESM_REGN_H
#define OPENMW_ESM_REGN_H

#include <array>
#include <map>
#include <string>
#include <vector>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    namespace Weather
    {
        // Clear, Cloudy, Foggy, Overcast, Rain, Thunder, Ash, Blight, Snow, Blizzard
        ESM::RefId indexToRefId(int index);
        int refIdToIndex(ESM::RefId id);

        constexpr int Length = 10;
    }

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
            std::map<RefId, uint8_t> mProbabilities;
        };

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
