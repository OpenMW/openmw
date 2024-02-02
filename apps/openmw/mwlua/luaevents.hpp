#ifndef MWLUA_LUAEVENTS_H
#define MWLUA_LUAEVENTS_H

#include <map>
#include <string>

#include <components/esm3/cellref.hpp> // defines RefNum that is used as a unique id

struct lua_State;

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace LuaUtil
{
    class UserdataSerializer;
}

namespace MWLua
{

    class GlobalScripts;
    class MenuScripts;

    class LuaEvents
    {
    public:
        explicit LuaEvents(GlobalScripts& globalScripts, MenuScripts& menuScripts)
            : mGlobalScripts(globalScripts)
            , mMenuScripts(menuScripts)
        {
        }

        struct Global
        {
            std::string mEventName;
            std::string mEventData;
        };
        struct Local
        {
            ESM::RefNum mDest;
            std::string mEventName;
            std::string mEventData;
        };

        void addGlobalEvent(Global event) { mNewGlobalEventBatch.push_back(std::move(event)); }
        void addMenuEvent(Global event) { mMenuEvents.push_back(std::move(event)); }
        void addLocalEvent(Local event) { mNewLocalEventBatch.push_back(std::move(event)); }

        void clear();
        void finalizeEventBatch();
        void callEventHandlers();
        void callMenuEventHandlers();

        void load(lua_State* lua, ESM::ESMReader& esm, const std::map<int, int>& contentFileMapping,
            const LuaUtil::UserdataSerializer* serializer);
        void save(ESM::ESMWriter& esm) const;

    private:
        GlobalScripts& mGlobalScripts;
        MenuScripts& mMenuScripts;
        std::vector<Global> mNewGlobalEventBatch;
        std::vector<Local> mNewLocalEventBatch;
        std::vector<Global> mGlobalEventBatch;
        std::vector<Local> mLocalEventBatch;
        std::vector<Global> mMenuEvents;
    };

}

#endif // MWLUA_LUAEVENTS_H
