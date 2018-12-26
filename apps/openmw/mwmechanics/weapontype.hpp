#ifndef GAME_MWMECHANICS_WEAPONTYPE_H
#define GAME_MWMECHANICS_WEAPONTYPE_H

#include "../mwworld/inventorystore.hpp"

#include "creaturestats.hpp"

namespace MWMechanics
{
    enum WeaponClass
    {
        Melee,
        Ranged,
        Thrown,
        Ammo
    };

    static const struct WeaponType
    {
        // Only one flag for now, later we can add other ones (e.g. for weapon parry)
        enum Flags
        {
            TwoHanded = 0x01
        };

        const int mType;
        const char mShortGroup[32];
        const char mLongGroup[32];
        const char mSoundId[32];
        const char mAttachBone[32];
        const char mSheathingBone[32];
        const ESM::Skill::SkillEnum mSkill;
        const WeaponClass mWeaponClass;
        const int mAmmoType;
        const int mFlags;
    }
    sWeaponTypeList[] =
    {
        { ESM::Weapon::None, "", "", "", "", "", ESM::Skill::HandToHand, WeaponClass::Melee, ESM::Weapon::None, 0 },
        { ESM::Weapon::PickProbe, "1h", "pickprobe", "", "", "", ESM::Skill::Security, WeaponClass::Melee, ESM::Weapon::None, 0 },
        { ESM::Weapon::Spell, "spell", "spellcast", "", "", "", ESM::Skill::HandToHand, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::HandToHand, "hh", "handtohand", "", "", "", ESM::Skill::HandToHand, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::ShortBladeOneHand, "1h", "weapononehand", "Item Weapon Shortblade", "Weapon Bone", "Bip01 ShortBladeOneHand", ESM::Skill::ShortBlade, WeaponClass::Melee, ESM::Weapon::None, 0 },
        { ESM::Weapon::LongBladeOneHand, "1h", "weapononehand", "Item Weapon Longblade", "Weapon Bone", "Bip01 LongBladeOneHand", ESM::Skill::LongBlade, WeaponClass::Melee, ESM::Weapon::None, 0 },
        { ESM::Weapon::BluntOneHand, "1h", "weapononehand", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntOneHand", ESM::Skill::BluntWeapon, WeaponClass::Melee, ESM::Weapon::None, 0 },
        { ESM::Weapon::AxeOneHand, "1h", "weapononehand", "Item Weapon Blunt", "Weapon Bone", "Bip01 LongBladeOneHand", ESM::Skill::Axe, WeaponClass::Melee, ESM::Weapon::None, 0 },
        { ESM::Weapon::LongBladeTwoHand, "2c", "weapontwohand", "Item Weapon Longblade", "Weapon Bone", "Bip01 LongBladeTwoClose", ESM::Skill::LongBlade, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::AxeTwoHand, "2c", "weapontwohand", "Item Weapon Blunt", "Weapon Bone", "Bip01 AxeTwoClose", ESM::Skill::Axe, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::BluntTwoClose, "2c", "weapontwohand", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntTwoClose", ESM::Skill::BluntWeapon, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::BluntTwoWide, "2w", "weapontwowide", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntTwoWide", ESM::Skill::BluntWeapon, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::SpearTwoWide, "2w", "weapontwowide", "Item Weapon Spear", "Weapon Bone", "Bip01 SpearTwoWide", ESM::Skill::Spear, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::MarksmanBow, "1h", "bowandarrow", "Item Weapon Bow", "Weapon Bone", "Bip01 MarksmanBow", ESM::Skill::Marksman, WeaponClass::Ranged, ESM::Weapon::Arrow, WeaponType::TwoHanded },
        { ESM::Weapon::MarksmanCrossbow, "crossbow", "crossbow", "Item Weapon Crossbow", "Weapon Bone", "Bip01 MarksmanCrossbow", ESM::Skill::Marksman, WeaponClass::Ranged, ESM::Weapon::Bolt, WeaponType::TwoHanded },
        { ESM::Weapon::MarksmanThrown, "1h", "throwweapon", "Item Weapon Blunt", "Weapon Bone", "Bip01 MarksmanThrown", ESM::Skill::Marksman, WeaponClass::Thrown, ESM::Weapon::None, WeaponType::TwoHanded },
        { ESM::Weapon::Arrow, "", "", "Item Weapon Ammo", "ArrowBone", "", ESM::Skill::Marksman, WeaponClass::Ammo, ESM::Weapon::None, 0 },
        { ESM::Weapon::Bolt, "", "", "Item Weapon Ammo", "ArrowBone", "", ESM::Skill::Marksman, WeaponClass::Ammo, ESM::Weapon::None, 0 }
    };

    MWWorld::ContainerStoreIterator getActiveWeapon(MWWorld::Ptr actor, int *weaptype);

    const WeaponType* getWeaponType(const int weaponType);
}

#endif
