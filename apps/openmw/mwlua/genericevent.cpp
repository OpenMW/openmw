#include "genericevent.hpp"

namespace MWLua
{
    namespace Event
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
