#include "eventqueue.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm/luascripts.hpp>

#include <components/lua/serialization.hpp>

namespace MWLua
{

    template <typename Event>
    void saveEvent(ESM::ESMWriter& esm, const ObjectId& dest, const Event& event)
    {
        esm.writeHNString("LUAE", event.mEventName);
        dest.save(esm, true);
        if (!event.mEventData.empty())
            saveLuaBinaryData(esm, event.mEventData);
    }

    void loadEvents(sol::state& lua, ESM::ESMReader& esm, GlobalEventQueue& globalEvents, LocalEventQueue& localEvents,
                    const std::map<int, int>& contentFileMapping, const LuaUtil::UserdataSerializer* serializer)
    {
        while (esm.isNextSub("LUAE"))
        {
            std::string name = esm.getHString();
            ObjectId dest;
            dest.load(esm, true);
            std::string data = loadLuaBinaryData(esm);
            try
            {
                data = LuaUtil::serialize(LuaUtil::deserialize(lua, data, serializer), serializer);
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "loadEvent: invalid event data: " << e.what();
            }
            if (dest.isSet())
            {
                auto it = contentFileMapping.find(dest.mContentFile);
                if (it != contentFileMapping.end())
                    dest.mContentFile = it->second;
                localEvents.push_back({dest, std::move(name), std::move(data)});
            }
            else
                globalEvents.push_back({std::move(name), std::move(data)});
        }
    }

    void saveEvents(ESM::ESMWriter& esm, const GlobalEventQueue& globalEvents, const LocalEventQueue& localEvents)
    {
        ObjectId globalId;
        globalId.unset();  // Used as a marker of a global event.

        for (const GlobalEvent& e : globalEvents)
            saveEvent(esm, globalId, e);
        for (const LocalEvent& e : localEvents)
            saveEvent(esm, e.mDest, e);
    }

}
