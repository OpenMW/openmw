#include "weapontype.hpp"

#include <components/debug/debuglog.hpp>

#include "../mwworld/class.hpp"

namespace MWMechanics
{
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

    void registerWeaponType(const std::string& type)
    {
        std::vector<std::string> weaponData;
        Misc::StringUtils::split(type, ';', weaponData);

        if (weaponData.size() < 11)
        {
            Log(Debug::Warning) << "Can not override weapon type info: can not parse string \"" << type << "\", ignore the override";
            return;
        }

        try
        {
            ESM::WeaponType weapon;
            weapon.mDisplayName = weaponData[1];
            weapon.mShortGroup = weaponData[2];
            weapon.mLongGroup = weaponData[3];
            weapon.mSoundId = weaponData[4];
            weapon.mAttachBone = weaponData[5];
            weapon.mSheathingBone = weaponData[6];
            weapon.mSkill = ESM::Skill::SkillEnum(std::stoi(weaponData[7]));
            weapon.mWeaponClass = ESM::WeaponType::Class(std::stoi(weaponData[8]));
            weapon.mAmmoType = std::stoi(weaponData[9]);
            weapon.mFlags = std::stoi(weaponData[10]);

            sWeaponTypeList[std::stoi(weaponData[0])] = weapon;
        }
        catch(const std::exception& e)
        {
            Log(Debug::Warning) << "Can not override weapon type info: can not parse string \"" << type << "\", ignore the override";
        }
    }
}
