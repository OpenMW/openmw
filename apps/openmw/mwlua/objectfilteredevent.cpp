
#include "objectfilteredevent.hpp"
#include "disableableevent.hpp"

#include "luamanager.hpp"
#include "util.hpp"

namespace MWLua
{
    namespace Event
    {
        ObjectFilteredEvent::ObjectFilteredEvent(const char* name, const MWWorld::Ptr& filter) :
            GenericEvent(name)//,
            //mEventFilter(filter)
        {
            // FIXME: just a stub now
        }

        sol::object ObjectFilteredEvent::getEventOptions()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table options = state.create_table();
            options["filter"] = makeLuaObject(mEventFilter);
            return options;
        }
    }
}
