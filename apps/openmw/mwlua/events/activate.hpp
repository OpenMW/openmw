#ifndef GAME_MWLUA_EVENT_ACTIVATE_H
#define GAME_MWLUA_EVENT_ACTIVATE_H

#include "../objectfilteredevent.hpp"
#include "../disableableevent.hpp"

#include "../../mwworld/ptr.hpp"

namespace MWLua
{
    namespace Event
    {
        class ActivateEvent : public ObjectFilteredEvent, public DisableableEvent<ActivateEvent>
        {
        public:
            ActivateEvent(const MWWorld::Ptr& activator, const MWWorld::Ptr& target);
            sol::table createEventTable();

        protected:
            MWWorld::Ptr mActivator;
            MWWorld::Ptr mTarget;
        };
    }
}

#endif
