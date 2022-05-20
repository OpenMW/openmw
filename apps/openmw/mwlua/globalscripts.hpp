#ifndef MWLUA_GLOBALSCRIPTS_H
#define MWLUA_GLOBALSCRIPTS_H

#include <memory>
#include <set>
#include <string>

#include <components/lua/luastate.hpp>
#include <components/lua/scriptscontainer.hpp>

#include "object.hpp"

namespace MWLua
{

    class GlobalScripts : public LuaUtil::ScriptsContainer
    {
    public:
        GlobalScripts(LuaUtil::LuaState* lua) :
            LuaUtil::ScriptsContainer(lua, "Global", ESM::LuaScriptCfg::sGlobal)
        {
            registerEngineHandlers({
                &mObjectActiveHandlers,
                &mActorActiveHandlers,
                &mItemActiveHandlers,
                &mNewGameHandlers,
                &mPlayerAddedHandlers
            });
        }

        void newGameStarted() { callEngineHandlers(mNewGameHandlers); }
        void objectActive(const GObject& obj) { callEngineHandlers(mObjectActiveHandlers, obj); }
        void actorActive(const GObject& obj) { callEngineHandlers(mActorActiveHandlers, obj); }
        void itemActive(const GObject& obj) { callEngineHandlers(mItemActiveHandlers, obj); }
        void playerAdded(const GObject& obj) { callEngineHandlers(mPlayerAddedHandlers, obj); }

    private:
        EngineHandlerList mObjectActiveHandlers{"onObjectActive"};
        EngineHandlerList mActorActiveHandlers{"onActorActive"};
        EngineHandlerList mItemActiveHandlers{"onItemActive"};
        EngineHandlerList mNewGameHandlers{"onNewGame"};
        EngineHandlerList mPlayerAddedHandlers{"onPlayerAdded"};
    };

}

#endif // MWLUA_GLOBALSCRIPTS_H
