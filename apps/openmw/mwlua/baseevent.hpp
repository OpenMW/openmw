#ifndef GAME_MWLUA_EVENT_H
#define GAME_MWLUA_EVENT_H

#include <extern/sol2/sol.hpp>

namespace MWLua
{
    namespace Event
    {
        sol::object trigger(const char* eventType, sol::table eventData = sol::nil, sol::object eventFilter = sol::nil);

        void clearObjectFilter(sol::object filterObject);

        class BaseEvent
        {
        public:
            virtual ~BaseEvent() {}
            virtual const char* getEventName() { return nullptr; };
            virtual sol::table createEventTable() { return sol::nil; };
            virtual sol::object getEventOptions() { return sol::nil; }
        };
    }
}

#endif
