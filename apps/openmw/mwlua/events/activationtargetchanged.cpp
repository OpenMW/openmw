#include "activationtargetchanged.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

namespace MWLua
{
    namespace Event
    {
        ActivationTargetChangedEvent::ActivationTargetChangedEvent(const MWWorld::Ptr& previous, const MWWorld::Ptr& current) :
            ObjectFilteredEvent("activationTargetChanged", current),
            mPreviousReference(previous),
            mCurrentReference(current)
        {

        }

        sol::table ActivationTargetChangedEvent::createEventTable()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table eventData = state.create_table();

            eventData["previous"] = makeLuaObject(mPreviousReference);
            eventData["current"] = makeLuaObject(mCurrentReference);

            return eventData;
        }

        //bool ActivationTargetChangedEvent::m_EventEnabled = false;
    }
}
