#ifndef MWLUA_EVENTQUEUE_H
#define MWLUA_EVENTQUEUE_H

#include "object.hpp"

namespace MWLua
{
    struct GlobalEvent
    {
        std::string eventName;
        std::string eventData;
    };
    struct LocalEvent
    {
        ObjectId dest;
        std::string eventName;
        std::string eventData;
    };
    using GlobalEventQueue = std::vector<GlobalEvent>;
    using LocalEventQueue = std::vector<LocalEvent>;
}

#endif // MWLUA_EVENTQUEUE_H
