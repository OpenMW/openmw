#ifndef GAME_MWLUA_GENERICEVENT_H
#define GAME_MWLUA_GENERICEVENT_H

#include "baseevent.hpp"

namespace MWLua
{
    namespace Event
    {
        // Generic event. Delivers no real payload and contains a dynamic name.
        class GenericEvent : public BaseEvent
        {
        public:
            GenericEvent(const char* name);
            const char* getEventName();

        protected:
            const char* mEventName;
        };
    }
}

#endif
