#ifndef GAME_MWMECHANICS_WEAPONTYPE_H
#define GAME_MWMECHANICS_WEAPONTYPE_H

#include "../mwworld/inventorystore.hpp"

#include <map>

#include "creaturestats.hpp"

namespace MWMechanics
{
    enum WeaponClass
    {
        Melee = 0,
        Ranged = 1,
        Thrown = 2,
        Ammo = 3
    };

    struct WeaponType
    {
        // Only one flag for now, later we can add other ones (e.g. for weapon parry)
        enum Flags
        {
            TwoHanded = 0x01
        };

        std::string mDisplayName;
        std::string mShortGroup;
        std::string mLongGroup;
        std::string mSoundId;
        std::string mAttachBone;
        std::string mSheathingBone;
        ESM::Skill::SkillEnum mSkill;
        WeaponClass mWeaponClass;
        int mAmmoType;
        int mFlags;
    };

    static std::map<int, WeaponType> sWeaponTypeList =
    {
        { ESM::Weapon::None, { "", "", "", "", "", "", ESM::Skill::HandToHand, WeaponClass::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::PickProbe, { "", "1h", "pickprobe", "", "", "", ESM::Skill::Security, WeaponClass::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::Spell, { "", "spell", "spellcast", "", "", "", ESM::Skill::HandToHand, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded }},
        { ESM::Weapon::HandToHand, { "", "hh", "handtohand", "", "", "", ESM::Skill::HandToHand, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded }},
        { ESM::Weapon::ShortBladeOneHand, { "Short Blade 1H", "1h", "weapononehand", "Item Weapon Shortblade", "Weapon Bone", "Bip01 ShortBladeOneHand", ESM::Skill::ShortBlade, WeaponClass::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::LongBladeOneHand, { "Long Blade 1H", "1h", "weapononehand", "Item Weapon Longblade", "Weapon Bone", "Bip01 LongBladeOneHand", ESM::Skill::LongBlade, WeaponClass::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::BluntOneHand, { "Blunt 1H", "1h", "weapononehand", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntOneHand", ESM::Skill::BluntWeapon, WeaponClass::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::AxeOneHand, { "Axe 1H", "1h", "weapononehand", "Item Weapon Blunt", "Weapon Bone", "Bip01 LongBladeOneHand", ESM::Skill::Axe, WeaponClass::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::LongBladeTwoHand, { "Long Blade 2H", "2c", "weapontwohand", "Item Weapon Longblade", "Weapon Bone", "Bip01 LongBladeTwoClose", ESM::Skill::LongBlade, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded }},
        { ESM::Weapon::AxeTwoHand, { "Axe 2H", "2c", "weapontwohand", "Item Weapon Blunt", "Weapon Bone", "Bip01 AxeTwoClose", ESM::Skill::Axe, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded }},
        { ESM::Weapon::BluntTwoClose, { "Blunt 2H Close", "2c", "weapontwohand", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntTwoClose", ESM::Skill::BluntWeapon, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded }},
        { ESM::Weapon::BluntTwoWide, { "Blunt 2H Wide", "2w", "weapontwowide", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntTwoWide", ESM::Skill::BluntWeapon, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded }},
        { ESM::Weapon::SpearTwoWide, { "Spear 2H", "2w", "weapontwowide", "Item Weapon Spear", "Weapon Bone", "Bip01 SpearTwoWide", ESM::Skill::Spear, WeaponClass::Melee, ESM::Weapon::None, WeaponType::TwoHanded }},
        { ESM::Weapon::MarksmanBow, { "Bow", "1h", "bowandarrow", "Item Weapon Bow", "Weapon Bone", "Bip01 MarksmanBow", ESM::Skill::Marksman, WeaponClass::Ranged, ESM::Weapon::Arrow, WeaponType::TwoHanded }},
        { ESM::Weapon::MarksmanCrossbow, { "Crossbow", "crossbow", "crossbow", "Item Weapon Crossbow", "Weapon Bone", "Bip01 MarksmanCrossbow", ESM::Skill::Marksman, WeaponClass::Ranged, ESM::Weapon::Bolt, WeaponType::TwoHanded }},
        { ESM::Weapon::MarksmanThrown, { "Thrown", "1h", "throwweapon", "Item Weapon Blunt", "Weapon Bone", "Bip01 MarksmanThrown", ESM::Skill::Marksman, WeaponClass::Thrown, ESM::Weapon::None, 0 }},
        { ESM::Weapon::Arrow, { "Arrow", "", "", "Item Weapon Ammo", "ArrowBone", "", ESM::Skill::Marksman, WeaponClass::Ammo, ESM::Weapon::None, 0 }},
        { ESM::Weapon::Bolt, { "Bolt", "", "", "Item Weapon Ammo", "ArrowBone", "", ESM::Skill::Marksman, WeaponClass::Ammo, ESM::Weapon::None, 0 }}
    };

    MWWorld::ContainerStoreIterator getActiveWeapon(MWWorld::Ptr actor, int *weaptype);

    const WeaponType* getWeaponType(const int weaponType);

    void registerWeaponType(const std::string& type);
}

#endif
