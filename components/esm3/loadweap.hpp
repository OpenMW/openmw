#ifndef OPENMW_ESM_WEAP_H
#define OPENMW_ESM_WEAP_H

#include <array>
#include <string>

#include <components/esm/path.hpp>
#include <components/esm/refid.hpp>

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Weapon definition
     */

    struct Weapon
    {
        constexpr static RecNameInts sRecordId = REC_WEAP;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Weapon"; }

        enum AttackType
        {
            AT_Chop,
            AT_Slash,
            AT_Thrust
        };

        enum Flags
        {
            Magical = 0x01,
            Silver = 0x02
        };

        struct WPDTstruct
        {
            float mWeight;
            int32_t mValue;
            RefId mType;
            uint16_t mHealth;
            float mSpeed, mReach;
            uint16_t mEnchant; // Enchantment points. The real value is mEnchant/10.f
            std::array<unsigned char, 2> mChop, mSlash, mThrust; // Min and max
            int32_t mFlags;
        }; // 32 bytes

        WPDTstruct mData;

        uint32_t mRecordFlags;
        RefId mId, mEnchant, mScript;
        std::string mName;
        Path mModel;
        Path mIcon;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        static RefId indexToRefId(int index);
        static int refIdToIndex(RefId id);

        void blank();
        ///< Set record to default state (does not touch the ID).
    };

    struct WeaponType
    {
        constexpr static RecNameInts sRecordId = REC_WTYP;
        static std::string_view getRecordType() { return "WeaponType"; }

        using TypeId = StringRefId;
        static const TypeId PickProbe;
        static const TypeId HandToHand;
        static const TypeId Spell;
        static const TypeId ShortBladeOneHand;
        static const TypeId LongBladeOneHand;
        static const TypeId LongBladeTwoHand;
        static const TypeId BluntOneHand;
        static const TypeId BluntTwoClose;
        static const TypeId BluntTwoWide;
        static const TypeId SpearTwoWide;
        static const TypeId AxeOneHand;
        static const TypeId AxeTwoHand;
        static const TypeId MarksmanBow;
        static const TypeId MarksmanCrossbow;
        static const TypeId MarksmanThrown;
        static const TypeId Arrow;
        static const TypeId Bolt;

        enum Flags
        {
            TwoHanded = 0x01,
            HasHealth = 0x02
        };

        enum Class
        {
            Melee = 0,
            Ranged = 1,
            Thrown = 2,
            Ammo = 3
        };

        ESM::RefId mId;
        // std::string mDisplayName; // TODO: will be needed later for editor
        std::string mShortGroup;
        std::string mLongGroup;
        ESM::RefId mSoundIdDown;
        ESM::RefId mSoundIdUp;
        std::string mAttachBone;
        std::string mSheathingBone;
        ESM::RefId mSkill;
        Class mWeaponClass = Melee;
        ESM::RefId mAmmoType;
        int mFlags = 0;

        WeaponType() = default;

        WeaponType(RefId id, std::string shortGroup, std::string longGroup, const std::string& soundId,
            std::string attachBone, std::string sheathingBone, ESM::RefId skill, Class weaponClass, ESM::RefId ammoType,
            int flags)
            : mId(std::move(id))
            , mShortGroup(std::move(shortGroup))
            , mLongGroup(std::move(longGroup))
            , mSoundIdDown(ESM::RefId::stringRefId(soundId + " Down"))
            , mSoundIdUp(ESM::RefId::stringRefId(soundId + " Up"))
            , mAttachBone(std::move(attachBone))
            , mSheathingBone(std::move(sheathingBone))
            , mSkill(skill)
            , mWeaponClass(weaponClass)
            , mAmmoType(std::move(ammoType))
            , mFlags(flags)
        {
        }

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;
    };

}
#endif
