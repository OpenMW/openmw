#ifndef GAME_MWMECHANICS_WEAPONTYPE_H
#define GAME_MWMECHANICS_WEAPONTYPE_H

#include "../mwworld/inventorystore.hpp"

#include <map>

#include "creaturestats.hpp"

namespace MWMechanics
{
    static std::map<int, ESM::WeaponType> sWeaponTypeList =
    {
        { ESM::Weapon::None, { "", "", "", "", "", "", ESM::Skill::HandToHand, ESM::WeaponType::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::PickProbe, { "", "1h", "pickprobe", "", "", "", ESM::Skill::Security, ESM::WeaponType::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::Spell, { "", "spell", "spellcast", "", "", "", ESM::Skill::HandToHand, ESM::WeaponType::Melee, ESM::Weapon::None, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::HandToHand, { "", "hh", "handtohand", "", "", "", ESM::Skill::HandToHand, ESM::WeaponType::Melee, ESM::Weapon::None, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::ShortBladeOneHand, { "Short Blade 1H", "1h", "weapononehand", "Item Weapon Shortblade", "Weapon Bone", "Bip01 ShortBladeOneHand", ESM::Skill::ShortBlade, ESM::WeaponType::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::LongBladeOneHand, { "Long Blade 1H", "1h", "weapononehand", "Item Weapon Longblade", "Weapon Bone", "Bip01 LongBladeOneHand", ESM::Skill::LongBlade, ESM::WeaponType::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::BluntOneHand, { "Blunt 1H", "1h", "weapononehand", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntOneHand", ESM::Skill::BluntWeapon, ESM::WeaponType::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::AxeOneHand, { "Axe 1H", "1h", "weapononehand", "Item Weapon Blunt", "Weapon Bone", "Bip01 LongBladeOneHand", ESM::Skill::Axe, ESM::WeaponType::Melee, ESM::Weapon::None, 0 }},
        { ESM::Weapon::LongBladeTwoHand, { "Long Blade 2H", "2c", "weapontwohand", "Item Weapon Longblade", "Weapon Bone", "Bip01 LongBladeTwoClose", ESM::Skill::LongBlade, ESM::WeaponType::Melee, ESM::Weapon::None, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::AxeTwoHand, { "Axe 2H", "2c", "weapontwohand", "Item Weapon Blunt", "Weapon Bone", "Bip01 AxeTwoClose", ESM::Skill::Axe, ESM::WeaponType::Melee, ESM::Weapon::None, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::BluntTwoClose, { "Blunt 2H Close", "2c", "weapontwohand", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntTwoClose", ESM::Skill::BluntWeapon, ESM::WeaponType::Melee, ESM::Weapon::None, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::BluntTwoWide, { "Blunt 2H Wide", "2w", "weapontwowide", "Item Weapon Blunt", "Weapon Bone", "Bip01 BluntTwoWide", ESM::Skill::BluntWeapon, ESM::WeaponType::Melee, ESM::Weapon::None, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::SpearTwoWide, { "Spear 2H", "2w", "weapontwowide", "Item Weapon Spear", "Weapon Bone", "Bip01 SpearTwoWide", ESM::Skill::Spear, ESM::WeaponType::Melee, ESM::Weapon::None, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::MarksmanBow, { "Bow", "1h", "bowandarrow", "Item Weapon Bow", "Weapon Bone", "Bip01 MarksmanBow", ESM::Skill::Marksman, ESM::WeaponType::Ranged, ESM::Weapon::Arrow, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::MarksmanCrossbow, { "Crossbow", "crossbow", "crossbow", "Item Weapon Crossbow", "Weapon Bone", "Bip01 MarksmanCrossbow", ESM::Skill::Marksman, ESM::WeaponType::Ranged, ESM::Weapon::Bolt, ESM::WeaponType::TwoHanded }},
        { ESM::Weapon::MarksmanThrown, { "Thrown", "1h", "throwweapon", "Item Weapon Blunt", "Weapon Bone", "Bip01 MarksmanThrown", ESM::Skill::Marksman, ESM::WeaponType::Thrown, ESM::Weapon::None, 0 }},
        { ESM::Weapon::Arrow, { "Arrow", "", "", "Item Weapon Ammo", "ArrowBone", "", ESM::Skill::Marksman, ESM::WeaponType::Ammo, ESM::Weapon::None, 0 }},
        { ESM::Weapon::Bolt, { "Bolt", "", "", "Item Weapon Ammo", "ArrowBone", "", ESM::Skill::Marksman, ESM::WeaponType::Ammo, ESM::Weapon::None, 0 }}
    };

    MWWorld::ContainerStoreIterator getActiveWeapon(MWWorld::Ptr actor, int *weaptype);

    const ESM::WeaponType* getWeaponType(const int weaponType);

    void registerWeaponType(const std::string& type);
}

#endif
