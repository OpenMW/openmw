#ifndef OPENMW_MWMECHANICS_ACTORUTIL_H
#define OPENMW_MWMECHANICS_ACTORUTIL_H

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

    template void setBaseAISetting<ESM::Creature>(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value);
    template void setBaseAISetting<ESM::NPC>(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value);
}

#endif
