#ifndef GAME_MWMECHANICS_WEAPONTYPE_H
#define GAME_MWMECHANICS_WEAPONTYPE_H

#include "../mwworld/inventorystore.hpp"

#include "creaturestats.hpp"

namespace MWMechanics
{
    static std::map<int, ESM::WeaponType> sWeaponTypeList =
    {
        {
            ESM::Weapon::None,
            {
                /* short group */ "",
                /* long group  */ "",
                /*  sound ID   */ "",
                /* attach bone */ "",
                /* sheath bone */ "",
                /* usage skill */ ESM::Skill::HandToHand,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ 0
            }
        },
        {
            ESM::Weapon::PickProbe,
            {
                /* short group */ "1h",
                /* long group  */ "pickprobe",
                /*  sound ID   */ "",
                /* attach bone */ "",
                /* sheath bone */ "",
                /* usage skill */ ESM::Skill::Security,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ 0
            }
        },
        {
            ESM::Weapon::Spell,
            {
                /* short group */ "spell",
                /* long group  */ "spellcast",
                /*  sound ID   */ "",
                /* attach bone */ "",
                /* sheath bone */ "",
                /* usage skill */ ESM::Skill::HandToHand,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::HandToHand,
            {
                /* short group */ "hh",
                /* long group  */ "handtohand",
                /*  sound ID   */ "",
                /* attach bone */ "",
                /* sheath bone */ "",
                /* usage skill */ ESM::Skill::HandToHand,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::ShortBladeOneHand,
            {
                /* short group */ "1s",
                /* long group  */ "shortbladeonehand",
                /*  sound ID   */ "Item Weapon Shortblade",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 ShortBladeOneHand",
                /* usage skill */ ESM::Skill::ShortBlade,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth
            }
        },
        {
            ESM::Weapon::LongBladeOneHand,
            {
                /* short group */ "1h",
                /* long group  */ "weapononehand",
                /*  sound ID   */ "Item Weapon Longblade",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 LongBladeOneHand",
                /* usage skill */ ESM::Skill::LongBlade,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth
            }
        },
        {
            ESM::Weapon::BluntOneHand,
            {
                /* short group */ "1b",
                /* long group  */ "bluntonehand",
                /*  sound ID   */ "Item Weapon Blunt",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 BluntOneHand",
                /* usage skill */ ESM::Skill::BluntWeapon,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth
            }
        },
        {
            ESM::Weapon::AxeOneHand,
            {
                /* short group */ "1b",
                /* long group  */ "bluntonehand",
                /*  sound ID   */ "Item Weapon Blunt",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 LongBladeOneHand",
                /* usage skill */ ESM::Skill::Axe,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth
            }
        },
        {
            ESM::Weapon::LongBladeTwoHand,
            {
                /* short group */ "2c",
                /* long group  */ "weapontwohand",
                /*  sound ID   */ "Item Weapon Longblade",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 LongBladeTwoClose",
                /* usage skill */ ESM::Skill::LongBlade,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::AxeTwoHand,
            {
                /* short group */ "2b",
                /* long group  */ "blunttwohand",
                /*  sound ID   */ "Item Weapon Blunt",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 AxeTwoClose",
                /* usage skill */ ESM::Skill::Axe,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::BluntTwoClose,
            {
                /* short group */ "2b",
                /* long group  */ "blunttwohand",
                /*  sound ID   */ "Item Weapon Blunt",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 BluntTwoClose",
                /* usage skill */ ESM::Skill::BluntWeapon,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::BluntTwoWide,
            {
                /* short group */ "2w",
                /* long group  */ "weapontwowide",
                /*  sound ID   */ "Item Weapon Blunt",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 BluntTwoWide",
                /* usage skill */ ESM::Skill::BluntWeapon,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::SpearTwoWide,
            {
                /* short group */ "2w",
                /* long group  */ "weapontwowide",
                /*  sound ID   */ "Item Weapon Spear",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 SpearTwoWide",
                /* usage skill */ ESM::Skill::Spear,
                /* weapon class*/ ESM::WeaponType::Melee,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::MarksmanBow,
            {
                /* short group */ "bow",
                /* long group  */ "bowandarrow",
                /*  sound ID   */ "Item Weapon Bow",
                /* attach bone */ "Weapon Bone Left",
                /* sheath bone */ "Bip01 MarksmanBow",
                /* usage skill */ ESM::Skill::Marksman,
                /* weapon class*/ ESM::WeaponType::Ranged,
                /*  ammo type  */ ESM::Weapon::Arrow,
                /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::MarksmanCrossbow,
            {
                /* short group */ "crossbow",
                /* long group  */ "crossbow",
                /*  sound ID   */ "Item Weapon Crossbow",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 MarksmanCrossbow",
                /* usage skill */ ESM::Skill::Marksman,
                /* weapon class*/ ESM::WeaponType::Ranged,
                /*  ammo type  */ ESM::Weapon::Bolt,
                /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
            }
        },
        {
            ESM::Weapon::MarksmanThrown,
            {
                /* short group */ "1t",
                /* long group  */ "throwweapon",
                /*  sound ID   */ "Item Weapon Blunt",
                /* attach bone */ "Weapon Bone",
                /* sheath bone */ "Bip01 MarksmanThrown",
                /* usage skill */ ESM::Skill::Marksman,
                /* weapon class*/ ESM::WeaponType::Thrown,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ 0
            }
        },
        {
            ESM::Weapon::Arrow,
            {
                /* short group */ "",
                /* long group  */ "",
                /*  sound ID   */ "Item Ammo",
                /* attach bone */ "ArrowBone",
                /* sheath bone */ "",
                /* usage skill */ ESM::Skill::Marksman,
                /* weapon class*/ ESM::WeaponType::Ammo,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ 0
            }
        },
        {
            ESM::Weapon::Bolt,
            {
                /* short group */ "",
                /* long group  */ "",
                /*  sound ID   */ "Item Ammo",
                /* attach bone */ "ArrowBone",
                /* sheath bone */ "",
                /* usage skill */ ESM::Skill::Marksman,
                /* weapon class*/ ESM::WeaponType::Ammo,
                /*  ammo type  */ ESM::Weapon::None,
                /*    flags    */ 0
            }
        }
    };

    MWWorld::ContainerStoreIterator getActiveWeapon(MWWorld::Ptr actor, int *weaptype);

    const ESM::WeaponType* getWeaponType(const int weaponType);
}

#endif
