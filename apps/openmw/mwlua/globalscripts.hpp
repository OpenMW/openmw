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
            registerEngineHandlers({&mActorActiveHandlers, &mNewGameHandlers, &mPlayerAddedHandlers});
        }

        void newGameStarted() { callEngineHandlers(mNewGameHandlers); }
        void actorActive(const GObject& obj) { callEngineHandlers(mActorActiveHandlers, obj); }
        void playerAdded(const GObject& obj) { callEngineHandlers(mPlayerAddedHandlers, obj); }

    private:
        EngineHandlerList mActorActiveHandlers{"onActorActive"};
        EngineHandlerList mNewGameHandlers{"onNewGame"};
        EngineHandlerList mPlayerAddedHandlers{"onPlayerAdded"};
    };

}

#endif // MWLUA_GLOBALSCRIPTS_H
