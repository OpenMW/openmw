#ifndef OPENMW_ESM_ENCH_H
#define OPENMW_ESM_ENCH_H

#include <cstdint>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "effectlist.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Enchantments
     */

    struct Enchantment
    {
        constexpr static RecNameInts sRecordId = REC_ENCH;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Enchantment"; }

        enum Type
        {
            CastOnce = 0,
            WhenStrikes = 1,
            WhenUsed = 2,
            ConstantEffect = 3
        };

        enum Flags
        {
            Autocalc = 0x01
        };

        struct ENDTstruct
        {
            int32_t mType;
            int32_t mCost;
            int32_t mCharge;
            int32_t mFlags;
        };

        uint32_t mRecordFlags;
        RefId mId;
        ENDTstruct mData;
        EffectList mEffects;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}
#endif
