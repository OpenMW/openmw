#include "weapontype.hpp"

#include "drawstate.hpp"
#include "creaturestats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include <components/esm3/loadweap.hpp>

namespace MWMechanics
{
    template <enum ESM::Weapon::Type>
    struct Weapon {};

    template <>
    struct Weapon<ESM::Weapon::None>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "",
            /* long group  */ "",
            /*  sound ID   */ "",
            /* attach bone */ "",
            /* sheath bone */ "",
            /* usage skill */ ESM::Skill::HandToHand,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ 0
        };
    };

    template <>
    struct Weapon<ESM::Weapon::PickProbe>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "1h",
            /* long group  */ "pickprobe",
            /*  sound ID   */ "",
            /* attach bone */ "",
            /* sheath bone */ "",
            /* usage skill */ ESM::Skill::Security,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ 0
        };
    };

    template <>
    struct Weapon<ESM::Weapon::Spell>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "spell",
            /* long group  */ "spellcast",
            /*  sound ID   */ "",
            /* attach bone */ "",
            /* sheath bone */ "",
            /* usage skill */ ESM::Skill::HandToHand,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::HandToHand>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "hh",
            /* long group  */ "handtohand",
            /*  sound ID   */ "",
            /* attach bone */ "",
            /* sheath bone */ "",
            /* usage skill */ ESM::Skill::HandToHand,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::ShortBladeOneHand>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "1s",
            /* long group  */ "shortbladeonehand",
            /*  sound ID   */ "Item Weapon Shortblade",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 ShortBladeOneHand",
            /* usage skill */ ESM::Skill::ShortBlade,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth
        };
    };

    template <>
    struct Weapon<ESM::Weapon::LongBladeOneHand>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "1h",
            /* long group  */ "weapononehand",
            /*  sound ID   */ "Item Weapon Longblade",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 LongBladeOneHand",
            /* usage skill */ ESM::Skill::LongBlade,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth
        };
    };

    template <>
    struct Weapon<ESM::Weapon::BluntOneHand>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "1b",
            /* long group  */ "bluntonehand",
            /*  sound ID   */ "Item Weapon Blunt",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 BluntOneHand",
            /* usage skill */ ESM::Skill::BluntWeapon,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth
        };
    };

    template <>
    struct Weapon<ESM::Weapon::AxeOneHand>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "1b",
            /* long group  */ "bluntonehand",
            /*  sound ID   */ "Item Weapon Blunt",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 LongBladeOneHand",
            /* usage skill */ ESM::Skill::Axe,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth
        };
    };

    template <>
    struct Weapon<ESM::Weapon::LongBladeTwoHand>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "2c",
            /* long group  */ "weapontwohand",
            /*  sound ID   */ "Item Weapon Longblade",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 LongBladeTwoClose",
            /* usage skill */ ESM::Skill::LongBlade,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::AxeTwoHand>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "2b",
            /* long group  */ "blunttwohand",
            /*  sound ID   */ "Item Weapon Blunt",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 AxeTwoClose",
            /* usage skill */ ESM::Skill::Axe,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::BluntTwoClose>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "2b",
            /* long group  */ "blunttwohand",
            /*  sound ID   */ "Item Weapon Blunt",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 BluntTwoClose",
            /* usage skill */ ESM::Skill::BluntWeapon,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::BluntTwoWide>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "2w",
            /* long group  */ "weapontwowide",
            /*  sound ID   */ "Item Weapon Blunt",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 BluntTwoWide",
            /* usage skill */ ESM::Skill::BluntWeapon,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::SpearTwoWide>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "2w",
            /* long group  */ "weapontwowide",
            /*  sound ID   */ "Item Weapon Spear",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 SpearTwoWide",
            /* usage skill */ ESM::Skill::Spear,
            /* weapon class*/ ESM::WeaponType::Melee,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::MarksmanBow>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "bow",
            /* long group  */ "bowandarrow",
            /*  sound ID   */ "Item Weapon Bow",
            /* attach bone */ "Weapon Bone Left",
            /* sheath bone */ "Bip01 MarksmanBow",
            /* usage skill */ ESM::Skill::Marksman,
            /* weapon class*/ ESM::WeaponType::Ranged,
            /*  ammo type  */ ESM::Weapon::Arrow,
            /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::MarksmanCrossbow>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "crossbow",
            /* long group  */ "crossbow",
            /*  sound ID   */ "Item Weapon Crossbow",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 MarksmanCrossbow",
            /* usage skill */ ESM::Skill::Marksman,
            /* weapon class*/ ESM::WeaponType::Ranged,
            /*  ammo type  */ ESM::Weapon::Bolt,
            /*    flags    */ ESM::WeaponType::HasHealth|ESM::WeaponType::TwoHanded
        };
    };

    template <>
    struct Weapon<ESM::Weapon::MarksmanThrown>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "1t",
            /* long group  */ "throwweapon",
            /*  sound ID   */ "Item Weapon Blunt",
            /* attach bone */ "Weapon Bone",
            /* sheath bone */ "Bip01 MarksmanThrown",
            /* usage skill */ ESM::Skill::Marksman,
            /* weapon class*/ ESM::WeaponType::Thrown,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ 0
        };
    };

    template <>
    struct Weapon<ESM::Weapon::Arrow>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "",
            /* long group  */ "",
            /*  sound ID   */ "Item Ammo",
            /* attach bone */ "Bip01 Arrow",
            /* sheath bone */ "",
            /* usage skill */ ESM::Skill::Marksman,
            /* weapon class*/ ESM::WeaponType::Ammo,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ 0
        };
    };

    template <>
    struct Weapon<ESM::Weapon::Bolt>
    {
        inline static const ESM::WeaponType sValue {
            /* short group */ "",
            /* long group  */ "",
            /*  sound ID   */ "Item Ammo",
            /* attach bone */ "ArrowBone",
            /* sheath bone */ "",
            /* usage skill */ ESM::Skill::Marksman,
            /* weapon class*/ ESM::WeaponType::Ammo,
            /*  ammo type  */ ESM::Weapon::None,
            /*    flags    */ 0
        };
    };

    MWWorld::ContainerStoreIterator getActiveWeapon(const MWWorld::Ptr& actor, int *weaptype)
    {
        MWWorld::InventoryStore &inv = actor.getClass().getInventoryStore(actor);
        CreatureStats &stats = actor.getClass().getCreatureStats(actor);
        if(stats.getDrawState() == MWMechanics::DrawState::Spell)
        {
            *weaptype = ESM::Weapon::Spell;
            return inv.end();
        }

        if(stats.getDrawState() == MWMechanics::DrawState::Weapon)
        {
            MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if(weapon == inv.end())
                *weaptype = ESM::Weapon::HandToHand;
            else
            {
                auto type = weapon->getType();
                if(type == ESM::Weapon::sRecordId)
                {
                    const MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
                    *weaptype = ref->mBase->mData.mType;
                }
                else if (type == ESM::Lockpick::sRecordId || type == ESM::Probe::sRecordId)
                    *weaptype = ESM::Weapon::PickProbe;
            }

            return weapon;
        }

        return inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    }

    const ESM::WeaponType* getWeaponType(const int weaponType)
    {
        switch (static_cast<ESM::Weapon::Type>(weaponType))
        {
            case ESM::Weapon::PickProbe: return &Weapon<ESM::Weapon::PickProbe>::sValue;
            case ESM::Weapon::HandToHand: return &Weapon<ESM::Weapon::HandToHand>::sValue;
            case ESM::Weapon::Spell: return &Weapon<ESM::Weapon::Spell>::sValue;
            case ESM::Weapon::None: return &Weapon<ESM::Weapon::None>::sValue;
            case ESM::Weapon::ShortBladeOneHand: return &Weapon<ESM::Weapon::ShortBladeOneHand>::sValue;
            case ESM::Weapon::LongBladeOneHand: return &Weapon<ESM::Weapon::LongBladeOneHand>::sValue;
            case ESM::Weapon::LongBladeTwoHand: return &Weapon<ESM::Weapon::LongBladeTwoHand>::sValue;
            case ESM::Weapon::BluntOneHand: return &Weapon<ESM::Weapon::BluntOneHand>::sValue;
            case ESM::Weapon::BluntTwoClose: return &Weapon<ESM::Weapon::BluntTwoClose>::sValue;
            case ESM::Weapon::BluntTwoWide: return &Weapon<ESM::Weapon::BluntTwoWide>::sValue;
            case ESM::Weapon::SpearTwoWide: return &Weapon<ESM::Weapon::SpearTwoWide>::sValue;
            case ESM::Weapon::AxeOneHand: return &Weapon<ESM::Weapon::AxeOneHand>::sValue;
            case ESM::Weapon::AxeTwoHand: return &Weapon<ESM::Weapon::AxeTwoHand>::sValue;
            case ESM::Weapon::MarksmanBow: return &Weapon<ESM::Weapon::MarksmanBow>::sValue;
            case ESM::Weapon::MarksmanCrossbow: return &Weapon<ESM::Weapon::MarksmanCrossbow>::sValue;
            case ESM::Weapon::MarksmanThrown: return &Weapon<ESM::Weapon::MarksmanThrown>::sValue;
            case ESM::Weapon::Arrow: return &Weapon<ESM::Weapon::Arrow>::sValue;
            case ESM::Weapon::Bolt: return &Weapon<ESM::Weapon::Bolt>::sValue;
        }

        return &Weapon<ESM::Weapon::ShortBladeOneHand>::sValue;
    }
}
