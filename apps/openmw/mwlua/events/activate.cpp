#include "activate.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

namespace MWLua
{
    namespace Event
    {
        ActivateEvent::ActivateEvent(const MWWorld::Ptr& activator, const MWWorld::Ptr& target) :
            ObjectFilteredEvent("activate", target),
            mActivator(activator),
            mTarget(target)
        {

        }

        sol::table ActivateEvent::createEventTable()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table eventData = state.create_table();

            eventData["activator"] = makeLuaObject(mActivator);
            eventData["target"] = makeLuaObject(mTarget);

            return eventData;
        }

        //bool ActivateEvent::m_EventEnabled = false;
    }
}
