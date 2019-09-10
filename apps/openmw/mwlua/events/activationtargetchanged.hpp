#ifndef GAME_MWLUA_EVENT_ACTIVATION_TARGET_CHANGED_H
#define GAME_MWLUA_EVENT_ACTIVATION_TARGET_CHANGED_H

#include "../objectfilteredevent.hpp"
#include "../disableableevent.hpp"

#include "../../mwworld/ptr.hpp"

namespace MWLua
{
    namespace Event
    {
        class ActivationTargetChangedEvent : public ObjectFilteredEvent, public DisableableEvent<ActivationTargetChangedEvent>
        {
            public:
                ActivationTargetChangedEvent(const MWWorld::Ptr& previous, const MWWorld::Ptr& current);
                sol::table createEventTable();

            protected:
            MWWorld::Ptr mPreviousReference;
            MWWorld::Ptr mCurrentReference;
        };
    }
}

#endif
