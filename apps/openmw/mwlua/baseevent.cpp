#include "baseevent.hpp"

#include "luamanager.hpp"

#include <components/debug/debuglog.hpp>

namespace mwse
{
    namespace lua
    {
        namespace event
        {
            sol::object trigger(const char* eventType, sol::table eventData, sol::object eventOptions)
            {
                auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
                sol::state& state = stateHandle.state;

                // Trigger the function, check for lua errors.
                sol::protected_function trigger = state["event"]["trigger"];
                sol::protected_function_result result = trigger(eventType, eventData, eventOptions);
                if (result.valid())
                {
                    return result;
                }
                else
                {
                    sol::error error = result;
                    Log(Debug::Error) << "Event system error encountered when raising " << eventType << " event:\n" << error.what();
                }

                return sol::nil;
            }

            void clearObjectFilter(sol::object filterObject)
            {
                auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
                sol::state& state = stateHandle.state;

                sol::protected_function trigger = state["event"]["clear"];
                trigger(sol::nil, filterObject);
            }
        }
    }
}
