#ifndef GAME_MWLUA_OBJECTFILTERED_EVENT_H
#define GAME_MWLUA_OBJECTFILTERED_EVENT_H

#include "genericevent.hpp"

#include "../mwworld/ptr.hpp"

namespace MWLua
{
    namespace Event
    {
        class ObjectFilteredEvent : public GenericEvent
        {
        public:
            ObjectFilteredEvent(const char* name, const MWWorld::Ptr& filter);
            sol::object getEventOptions();

        protected:
             MWWorld::Ptr mEventFilter;
        };
    }
}

#endif
