#ifndef OPENMW_ESM_SKIL_H
#define OPENMW_ESM_SKIL_H

#include <array>
#include <optional>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    struct MagicSchool
    {
        ESM::RefId mAreaSound;
        ESM::RefId mBoltSound;
        ESM::RefId mCastSound;
        ESM::RefId mFailureSound;
        ESM::RefId mHitSound;
        std::string mName;
        int mAutoCalcMax;

        static constexpr int Length = 6;

        static RefId indexToSkillRefId(int index);
        static int skillRefIdToIndex(RefId id);
    };

    /*
     * Skill information
     *
     */

    struct Skill
    {

        constexpr static RecNameInts sRecordId = REC_SKIL;
        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Skill"; }

        unsigned int mRecordFlags;
        RefId mId;

        struct SKDTstruct
        {
            int mAttribute; // see defs.hpp
            int mSpecialization; // 0 - Combat, 1 - Magic, 2 - Stealth
            float mUseValue[4]; // How much skill improves through use. Meaning
                                // of each field depends on what skill this
                                // is. We should document this better later.
        }; // Total size: 24 bytes
        SKDTstruct mData;

        // Skill index. Skils don't have an id ("NAME") like most records,
        // they only have a numerical index that matches one of the
        // hard-coded skills in the game.
        int mIndex{ -1 };

        std::string mDescription;
        std::string mName;
        std::string mIcon;
        float mWerewolfValue{};
        std::optional<MagicSchool> mSchool;

        static constexpr IndexRefId Block{ sRecordId, 0 };
        static constexpr IndexRefId Armorer{ sRecordId, 1 };
        static constexpr IndexRefId MediumArmor{ sRecordId, 2 };
        static constexpr IndexRefId HeavyArmor{ sRecordId, 3 };
        static constexpr IndexRefId BluntWeapon{ sRecordId, 4 };
        static constexpr IndexRefId LongBlade{ sRecordId, 5 };
        static constexpr IndexRefId Axe{ sRecordId, 6 };
        static constexpr IndexRefId Spear{ sRecordId, 7 };
        static constexpr IndexRefId Athletics{ sRecordId, 8 };
        static constexpr IndexRefId Enchant{ sRecordId, 9 };
        static constexpr IndexRefId Destruction{ sRecordId, 10 };
        static constexpr IndexRefId Alteration{ sRecordId, 11 };
        static constexpr IndexRefId Illusion{ sRecordId, 12 };
        static constexpr IndexRefId Conjuration{ sRecordId, 13 };
        static constexpr IndexRefId Mysticism{ sRecordId, 14 };
        static constexpr IndexRefId Restoration{ sRecordId, 15 };
        static constexpr IndexRefId Alchemy{ sRecordId, 16 };
        static constexpr IndexRefId Unarmored{ sRecordId, 17 };
        static constexpr IndexRefId Security{ sRecordId, 18 };
        static constexpr IndexRefId Sneak{ sRecordId, 19 };
        static constexpr IndexRefId Acrobatics{ sRecordId, 20 };
        static constexpr IndexRefId LightArmor{ sRecordId, 21 };
        static constexpr IndexRefId ShortBlade{ sRecordId, 22 };
        static constexpr IndexRefId Marksman{ sRecordId, 23 };
        static constexpr IndexRefId Mercantile{ sRecordId, 24 };
        static constexpr IndexRefId Speechcraft{ sRecordId, 25 };
        static constexpr IndexRefId HandToHand{ sRecordId, 26 };
        static constexpr int Length = 27;
        static const std::string sSkillNames[Length];

        static int stringToSkillId(std::string_view skill);

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).

        static RefId indexToRefId(int index);
    };
}
#endif
