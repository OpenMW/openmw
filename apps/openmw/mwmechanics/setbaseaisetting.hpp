#ifndef OPENMW_MWMECHANICS_SETBASEAISETTING_H
#define OPENMW_MWMECHANICS_SETBASEAISETTING_H

#include <string>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include "aisetting.hpp"
#include "creaturestats.hpp"

namespace MWMechanics
{
    template<class T>
    void setBaseAISetting(const std::string& id, MWMechanics::AiSetting setting, int value)
    {
        T copy = *MWBase::Environment::get().getWorld()->getStore().get<T>().find(id);
        switch (setting)
        {
            case MWMechanics::AiSetting::Hello:
                copy.mAiData.mHello = value;
                break;
            case MWMechanics::AiSetting::Fight:
                copy.mAiData.mFight = value;
                break;
            case MWMechanics::AiSetting::Flee:
                copy.mAiData.mFlee = value;
                break;
            case MWMechanics::AiSetting::Alarm:
                copy.mAiData.mAlarm = value;
                break;
            default:
                assert(false);
        }
        MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
    }
}

#endif
