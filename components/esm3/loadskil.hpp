#ifndef OPENMW_ESM_SKIL_H
#define OPENMW_ESM_SKIL_H

#include <array>
#include <optional>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/path.hpp"
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
        int32_t mAutoCalcMax;

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

        uint32_t mRecordFlags;
        RefId mId;

        //! Enum that defines the index into SKDTstruct::mUseValue for all vanilla skill uses
        enum UseType
        {
            // These are shared by multiple skills
            Armor_HitByOpponent = 0,
            Block_Success = 0,
            Spellcast_Success = 0,
            Weapon_SuccessfulHit = 0,

            // Skill-specific use types
            Alchemy_CreatePotion = 0,
            Alchemy_UseIngredient = 1,
            Enchant_Recharge = 0,
            Enchant_UseMagicItem = 1,
            Enchant_CreateMagicItem = 2,
            Enchant_CastOnStrike = 3,
            Acrobatics_Jump = 0,
            Acrobatics_Fall = 1,
            Mercantile_Success = 0,
            Mercantile_Bribe = 1, //!< \Note This is bugged in vanilla and is not actually in use.
            Security_DisarmTrap = 0,
            Security_PickLock = 1,
            Sneak_AvoidNotice = 0,
            Sneak_PickPocket = 1,
            Speechcraft_Success = 0,
            Speechcraft_Fail = 1,
            Armorer_Repair = 0,
            Athletics_RunOneSecond = 0,
            Athletics_SwimOneSecond = 1,

        };

        struct SKDTstruct
        {
            ESM::RefId mAttribute; // see defs.hpp
            int32_t mSpecialization; // 0 - Combat, 1 - Magic, 2 - Stealth
            std::array<float, 4> mUseValue; // How much skill improves through use. Meaning
                                            // of each field depends on what skill this
                                            // is. See UseType above
        }; // Total size: 24 bytes
        SKDTstruct mData;

        std::string mDescription;
        std::string mName;
        Path mIcon;
        float mWerewolfValue{};
        std::optional<MagicSchool> mSchool;

        static const RefId Block;
        static const RefId Armorer;
        static const RefId MediumArmor;
        static const RefId HeavyArmor;
        static const RefId BluntWeapon;
        static const RefId LongBlade;
        static const RefId Axe;
        static const RefId Spear;
        static const RefId Athletics;
        static const RefId Enchant;
        static const RefId Destruction;
        static const RefId Alteration;
        static const RefId Illusion;
        static const RefId Conjuration;
        static const RefId Mysticism;
        static const RefId Restoration;
        static const RefId Alchemy;
        static const RefId Unarmored;
        static const RefId Security;
        static const RefId Sneak;
        static const RefId Acrobatics;
        static const RefId LightArmor;
        static const RefId ShortBlade;
        static const RefId Marksman;
        static const RefId Mercantile;
        static const RefId Speechcraft;
        static const RefId HandToHand;
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
