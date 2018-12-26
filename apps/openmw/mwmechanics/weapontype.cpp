#include "weapontype.hpp"

#include "../mwworld/class.hpp"

namespace MWMechanics
{
    static const WeaponType *sWeaponTypeListEnd = &sWeaponTypeList[sizeof(sWeaponTypeList)/sizeof(sWeaponTypeList[0])];

    class FindWeaponType
    {
        int type;

    public:
        FindWeaponType(int _type) : type(_type) { }

        bool operator()(const WeaponType &weap) const
        { return weap.mType == type; }
    };

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

    const WeaponType* getWeaponType(const int weaponType)
    {
        const WeaponType *weap = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(weaponType));
        if (weap == sWeaponTypeListEnd)
        {
            // Use one-handed short blades as fallback
            const WeaponType *fallback = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(ESM::Weapon::ShortBladeOneHand));
            return fallback;
        }

        return weap;
    }
}
