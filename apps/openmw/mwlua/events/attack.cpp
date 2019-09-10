#include "attack.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

namespace MWLua
{
    namespace Event
    {
        AttackEvent::AttackEvent(const MWWorld::Ptr& attacker, const MWWorld::Ptr& target) :
            ObjectFilteredEvent("attack", target),
            mAttacker(attacker),
            mTarget(target)
        {

        }

        sol::table AttackEvent::createEventTable()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table eventData = state.create_table();

            //eventData["mobile"] = makeLuaObject(m_AnimationData->mobileActor);
            eventData["reference"] = makeLuaObject(mAttacker);

            if (!mTarget.isEmpty())
            {
                //eventData["targetMobile"] = makeLuaObject(target);
                eventData["targetReference"] = makeLuaObject(mTarget);
            }

            return eventData;
        }

        //bool AttackEvent::m_EventEnabled = false;
    }
}
