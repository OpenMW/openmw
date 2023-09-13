#ifndef OPENMW_ESM_LIGH_H
#define OPENMW_ESM_LIGH_H

#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Lights. Includes static light sources and also carryable candles
     * and torches.
     */

    struct Light
    {
        constexpr static RecNameInts sRecordId = REC_LIGH;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Light"; }

        enum Flags
        {
            Dynamic = 0x001,
            Carry = 0x002, // Can be carried
            Negative = 0x004, // Negative light - i.e. darkness
            Flicker = 0x008,
            Fire = 0x010,
            OffDefault
            = 0x020, // Off by default - does not burn while placed in a cell, but can burn when equipped by an NPC
            FlickerSlow = 0x040,
            Pulse = 0x080,
            PulseSlow = 0x100
        };

        struct LHDTstruct
        {
            float mWeight;
            int32_t mValue;
            int32_t mTime; // Duration
            int32_t mRadius;
            uint32_t mColor; // 4-byte rgba value
            int32_t mFlags;
        }; // Size = 24 bytes

        LHDTstruct mData;

        uint32_t mRecordFlags;
        std::string mModel, mIcon, mName;
        ESM::RefId mId, mSound, mScript;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}
#endif
