#pragma once

#include "genericevent.hpp"

namespace mwse
{
    namespace lua
    {
        namespace event
        {
            GenericEvent::GenericEvent(const char* name) :
                mEventName(name)
            {

            }

            const char* GenericEvent::getEventName()
            {
                return mEventName;
            }
        }
    }
}
