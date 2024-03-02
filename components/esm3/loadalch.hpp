#ifndef OPENMW_ESM_ALCH_H
#define OPENMW_ESM_ALCH_H

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
     * Alchemy item (potions)
     */

    struct Potion
    {
        constexpr static RecNameInts sRecordId = REC_ALCH;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Potion"; }

        enum Flags
        {
            Autocalc = 1 // Determines value
        };

        struct ALDTstruct
        {
            float mWeight;
            int32_t mValue;
            int32_t mFlags;
        };
        ALDTstruct mData;

        uint32_t mRecordFlags;
        RefId mId, mScript;
        std::string mName, mModel, mIcon;
        EffectList mEffects;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };
}
#endif
