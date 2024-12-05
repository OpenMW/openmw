#include "luaevents.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm/luascripts.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/lua/serialization.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/worldmodel.hpp"

#include "globalscripts.hpp"
#include "localscripts.hpp"
#include "menuscripts.hpp"

namespace MWLua
{

    void LuaEvents::clear()
    {
        mGlobalEventBatch.clear();
        mLocalEventBatch.clear();
        mNewGlobalEventBatch.clear();
        mNewLocalEventBatch.clear();
        mMenuEvents.clear();
    }

    void LuaEvents::finalizeEventBatch()
    {
        mNewGlobalEventBatch.swap(mGlobalEventBatch);
        mNewLocalEventBatch.swap(mLocalEventBatch);
        mNewGlobalEventBatch.clear();
        mNewLocalEventBatch.clear();
    }

    void LuaEvents::callEventHandlers()
    {
        for (const Global& e : mGlobalEventBatch)
            mGlobalScripts.receiveEvent(e.mEventName, e.mEventData);
        mGlobalEventBatch.clear();
        for (const Local& e : mLocalEventBatch)
        {
            MWWorld::Ptr ptr = MWBase::Environment::get().getWorldModel()->getPtr(e.mDest);
            LocalScripts* scripts = ptr.isEmpty() ? nullptr : ptr.getRefData().getLuaScripts();
            if (scripts)
                scripts->receiveEvent(e.mEventName, e.mEventData);
            else
                Log(Debug::Debug) << "Ignored event " << e.mEventName << " to L" << e.mDest.toString()
                                  << ". Object not found or has no attached scripts";
        }
        mLocalEventBatch.clear();
    }

    void LuaEvents::callMenuEventHandlers()
    {
        for (const Global& e : mMenuEvents)
            mMenuScripts.receiveEvent(e.mEventName, e.mEventData);
        mMenuEvents.clear();
    }

    template <typename Event>
    static void saveEvent(ESM::ESMWriter& esm, ESM::RefNum dest, const Event& event)
    {
        esm.writeHNString("LUAE", event.mEventName);
        esm.writeFormId(dest, true);
        if (!event.mEventData.empty())
            saveLuaBinaryData(esm, event.mEventData);
    }

    void LuaEvents::load(lua_State* lua, ESM::ESMReader& esm, const std::map<int, int>& contentFileMapping,
        const LuaUtil::UserdataSerializer* serializer)
    {
        clear();
        while (esm.isNextSub("LUAE"))
        {
            std::string name = esm.getHString();
            ESM::RefNum dest = esm.getFormId(true);
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
                mLocalEventBatch.push_back({ dest, std::move(name), std::move(data) });
            }
            else
                mGlobalEventBatch.push_back({ std::move(name), std::move(data) });
        }
    }

    void LuaEvents::save(ESM::ESMWriter& esm) const
    {
        // Used as a marker of a global event.
        constexpr ESM::RefNum globalId;

        for (const Global& e : mGlobalEventBatch)
            saveEvent(esm, globalId, e);
        for (const Global& e : mNewGlobalEventBatch)
            saveEvent(esm, globalId, e);
        for (const Local& e : mLocalEventBatch)
            saveEvent(esm, e.mDest, e);
        for (const Local& e : mNewLocalEventBatch)
            saveEvent(esm, e.mDest, e);
    }

}
