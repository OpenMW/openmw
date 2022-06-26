#ifndef OPENMW_MWMECHANICS_AISETTING_H
#define OPENMW_MWMECHANICS_AISETTING_H

#include <string>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include "creaturestats.hpp"

namespace MWMechanics
{
    template<class T>
    void setBaseAISetting(const std::string& id, MWMechanics::CreatureStats::AiSetting setting, int value)
    {
        T copy = *MWBase::Environment::get().getWorld()->getStore().get<T>().find(id);
        switch (setting)
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
                assert(false);
        }
        MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
    }
}

#endif
