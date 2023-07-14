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

    using SkillId = StringRefId;

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
        SkillId mId;

        struct SKDTstruct
        {
            int mAttribute; // see defs.hpp
            int mSpecialization; // 0 - Combat, 1 - Magic, 2 - Stealth
            float mUseValue[4]; // How much skill improves through use. Meaning
                                // of each field depends on what skill this
                                // is. We should document this better later.
        }; // Total size: 24 bytes
        SKDTstruct mData;

        std::string mDescription;
        std::string mName;
        std::string mIcon;
        float mWerewolfValue{};
        std::optional<MagicSchool> mSchool;

        static const SkillId Block;
        static const SkillId Armorer;
        static const SkillId MediumArmor;
        static const SkillId HeavyArmor;
        static const SkillId BluntWeapon;
        static const SkillId LongBlade;
        static const SkillId Axe;
        static const SkillId Spear;
        static const SkillId Athletics;
        static const SkillId Enchant;
        static const SkillId Destruction;
        static const SkillId Alteration;
        static const SkillId Illusion;
        static const SkillId Conjuration;
        static const SkillId Mysticism;
        static const SkillId Restoration;
        static const SkillId Alchemy;
        static const SkillId Unarmored;
        static const SkillId Security;
        static const SkillId Sneak;
        static const SkillId Acrobatics;
        static const SkillId LightArmor;
        static const SkillId ShortBlade;
        static const SkillId Marksman;
        static const SkillId Mercantile;
        static const SkillId Speechcraft;
        static const SkillId HandToHand;
        static constexpr int Length = 27;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).

        static RefId indexToRefId(int index);
        static int refIdToIndex(RefId id);
    };
}
#endif
