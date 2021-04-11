#ifndef OPENMW_MWMECHANICS_ACTORUTIL_H
#define OPENMW_MWMECHANICS_ACTORUTIL_H

#include <algorithm>

#include <components/esm/loadcont.hpp>
#include <components/esm/loadcrea.hpp>
#include <components/esm/loadnpc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include "./creaturestats.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    enum GreetingState
    {
        Greet_None,
        Greet_InProgress,
        Greet_Done
    };

    MWWorld::Ptr getPlayer();
    bool isPlayerInCombat();
    bool canActorMoveByZAxis(const MWWorld::Ptr& actor);
    bool hasWaterWalking(const MWWorld::Ptr& actor);

    template<class T>
    void setBaseAISetting(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value)
    {
        T copy = *MWBase::Environment::get().getWorld()->getStore().get<T>().find(id);
        switch(setting)
        {
            case MWMechanics::CreatureStats::AiSetting::AI_Hello:
                copy.mAiData.mHello = value;
                break;
            case MWMechanics::CreatureStats::AiSetting::AI_Fight:
                copy.mAiData.mFight = value;
                break;
            case MWMechanics::CreatureStats::AiSetting::AI_Flee:
                copy.mAiData.mFlee = value;
                break;
            case MWMechanics::CreatureStats::AiSetting::AI_Alarm:
                copy.mAiData.mAlarm = value;
                break;
            default:
                assert(0);
        }
        MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
    }

    template<class T>
    void modifyBaseInventory(const std::string& actorId, const std::string& itemId, int amount)
    {
        T copy = *MWBase::Environment::get().getWorld()->getStore().get<T>().find(actorId);
        for(auto& it : copy.mInventory.mList)
        {
            if(Misc::StringUtils::ciEqual(it.mItem, itemId))
            {
                int sign = it.mCount < 1 ? -1 : 1;
                it.mCount = sign * std::max(it.mCount * sign + amount, 0);
                MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
                return;
            }
        }
        if(amount > 0)
        {
            ESM::ContItem cont;
            cont.mItem = itemId;
            cont.mCount = amount;
            copy.mInventory.mList.push_back(cont);
            MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
        }
    }

    template void setBaseAISetting<ESM::Creature>(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value);
    template void setBaseAISetting<ESM::NPC>(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value);
    template void modifyBaseInventory<ESM::Creature>(const std::string& actorId, const std::string& itemId, int amount);
    template void modifyBaseInventory<ESM::NPC>(const std::string& actorId, const std::string& itemId, int amount);
    template void modifyBaseInventory<ESM::Container>(const std::string& containerId, const std::string& itemId, int amount);
}

#endif
