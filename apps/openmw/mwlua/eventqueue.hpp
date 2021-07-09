#ifndef MWLUA_EVENTQUEUE_H
#define MWLUA_EVENTQUEUE_H

#include "object.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace LuaUtil
{
    class UserdataSerializer;
}

namespace sol
{
    class state;
}

namespace MWLua
{
    struct GlobalEvent
    {
        std::string mEventName;
        std::string mEventData;
    };
    struct LocalEvent
    {
        ObjectId mDest;
        std::string mEventName;
        std::string mEventData;
    };
    using GlobalEventQueue = std::vector<GlobalEvent>;
    using LocalEventQueue = std::vector<LocalEvent>;

    void loadEvents(sol::state& lua, ESM::ESMReader& esm, GlobalEventQueue&, LocalEventQueue&,
                    const std::map<int, int>& contentFileMapping, const LuaUtil::UserdataSerializer* serializer);
    void saveEvents(ESM::ESMWriter& esm, const GlobalEventQueue&, const LocalEventQueue&);
}

#endif // MWLUA_EVENTQUEUE_H
