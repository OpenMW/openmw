#ifndef OPENMW_ESM_LEVLISTS_H
#define OPENMW_ESM_LEVLISTS_H

#include <string>
#include <vector>

#include "components/esm/refid.hpp"
#include <components/esm/defs.hpp>
#include <components/esm/esmcommon.hpp>

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Levelled lists. Since these have identical layout, I only bothered
     * to implement it once.
     *
     * We should later implement the ability to merge levelled lists from
     * several files.
     */

    struct LevelledListBase
    {
        int32_t mFlags;
        unsigned char mChanceNone; // Chance that none are selected (0-100)
        uint32_t mRecordFlags;
        RefId mId;

        struct LevelItem
        {
            RefId mId;
            uint16_t mLevel;
        };

        std::vector<LevelItem> mList;

        void load(ESMReader& esm, NAME recName, bool& isDeleted);
        void save(ESMWriter& esm, NAME recName, bool isDeleted) const;

        void blank();
        ///< Set record to default state (does not touch the ID).
    };

    template <class Base>
    struct CustomLevelledListBase : LevelledListBase
    {
        void load(ESMReader& esm, bool& isDeleted) { LevelledListBase::load(esm, Base::sRecName, isDeleted); }
        void save(ESMWriter& esm, bool isDeleted = false) const
        {
            LevelledListBase::save(esm, Base::sRecName, isDeleted);
        }
    };

    struct CreatureLevList : CustomLevelledListBase<CreatureLevList>
    {
        /// Record name used to read references.
        static constexpr NAME sRecName{ "CNAM" };
        static constexpr RecNameInts sRecordId = RecNameInts::REC_LEVC;
        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "CreatureLevList"; }

        enum Flags
        {

            AllLevels = 0x01 // Calculate from all levels <= player
                             // level, not just the closest below
                             // player.
        };
    };

    struct ItemLevList : CustomLevelledListBase<ItemLevList>
    {
        /// Record name used to read references.
        static constexpr NAME sRecName{ "INAM" };
        static constexpr RecNameInts sRecordId = RecNameInts::REC_LEVI;
        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "ItemLevList"; }

        enum Flags
        {

            Each = 0x01, // Select a new item each time this
                         // list is instantiated, instead of
                         // giving several identical items
                         // (used when a container has more
                         // than one instance of one levelled
                         // list.)
            AllLevels = 0x02 // Calculate from all levels <= player
                             // level, not just the closest below
                             // player.
        };
    };

}
#endif
