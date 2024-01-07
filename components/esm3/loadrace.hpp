#ifndef OPENMW_ESM_RACE_H
#define OPENMW_ESM_RACE_H

#include <array>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "spelllist.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Race definition
     */

    struct Race
    {
        constexpr static RecNameInts sRecordId = REC_RACE;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Race"; }

        struct SkillBonus
        {
            int32_t mSkill; // SkillEnum
            int32_t mBonus;
        };

        enum Flags
        {
            Playable = 0x01,
            Beast = 0x02
        };

        struct RADTstruct
        {
            // List of skills that get a bonus
            std::array<SkillBonus, 7> mBonus;

            // Attribute values for male/female
            std::array<int32_t, 16> mAttributeValues;

            // The actual eye level height (in game units) is (probably) given
            // as 'height' times 128. This has not been tested yet.
            float mMaleHeight, mFemaleHeight, mMaleWeight, mFemaleWeight;

            int32_t mFlags; // 0x1 - playable, 0x2 - beast race

            int32_t getAttribute(ESM::RefId attribute, bool male) const;
            void setAttribute(ESM::RefId attribute, bool male, int32_t value);

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;

        }; // Size = 140 bytes

        RADTstruct mData;

        uint32_t mRecordFlags;
        std::string mName, mDescription;
        RefId mId;
        SpellList mPowers;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).
    };

}
#endif
