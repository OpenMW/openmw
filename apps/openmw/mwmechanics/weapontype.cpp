#include "weapontype.hpp"

#include "../mwworld/class.hpp"

namespace MWMechanics
{
    static const ESM::WeaponType *sWeaponTypeListEnd = &sWeaponTypeList[sizeof(sWeaponTypeList)/sizeof(sWeaponTypeList[0])];

    MWWorld::ContainerStoreIterator getActiveWeapon(MWWorld::Ptr actor, int *weaptype)
    {
        MWWorld::InventoryStore &inv = actor.getClass().getInventoryStore(actor);
        CreatureStats &stats = actor.getClass().getCreatureStats(actor);
        if(stats.getDrawState() == MWMechanics::DrawState_Spell)
        {
            *weaptype = ESM::Weapon::Spell;
            return inv.end();
        }

        if(stats.getDrawState() == MWMechanics::DrawState_Weapon)
        {
            MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if(weapon == inv.end())
                *weaptype = ESM::Weapon::HandToHand;
            else
            {
                const std::string &type = weapon->getTypeName();
                if(type == typeid(ESM::Weapon).name())
                {
                    const MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
                    *weaptype = ref->mBase->mData.mType;
                }
                else if (type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
                    *weaptype = ESM::Weapon::PickProbe;
            }

            return weapon;
        }

        return inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
    }

    const ESM::WeaponType* getWeaponType(const int weaponType)
    {
        std::map<int, ESM::WeaponType>::const_iterator found = sWeaponTypeList.find(weaponType);
        if (found == sWeaponTypeList.end())
        {
            // Use one-handed short blades as fallback
            return &sWeaponTypeList[0];
        }

         return &found->second;
    }
}
