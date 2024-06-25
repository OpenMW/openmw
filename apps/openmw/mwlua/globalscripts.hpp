#ifndef MWLUA_GLOBALSCRIPTS_H
#define MWLUA_GLOBALSCRIPTS_H

#include <components/lua/luastate.hpp>
#include <components/lua/scriptscontainer.hpp>

#include "object.hpp"

namespace MWLua
{

    class GlobalScripts : public LuaUtil::ScriptsContainer
    {
    public:
        GlobalScripts(LuaUtil::LuaState* lua)
            : LuaUtil::ScriptsContainer(lua, "Global")
        {
            registerEngineHandlers({
                &mObjectActiveHandlers,
                &mActorActiveHandlers,
                &mItemActiveHandlers,
                &mNewGameHandlers,
                &mPlayerAddedHandlers,
                &mOnActivateHandlers,
                &mOnUseItemHandlers,
                &mOnNewExteriorHandlers,
            });
        }

        void newGameStarted() { callEngineHandlers(mNewGameHandlers); }
        void objectActive(const GObject& obj) { callEngineHandlers(mObjectActiveHandlers, obj); }
        void actorActive(const GObject& obj) { callEngineHandlers(mActorActiveHandlers, obj); }
        void itemActive(const GObject& obj) { callEngineHandlers(mItemActiveHandlers, obj); }
        void playerAdded(const GObject& obj) { callEngineHandlers(mPlayerAddedHandlers, obj); }
        void onActivate(const GObject& obj, const GObject& actor)
        {
            callEngineHandlers(mOnActivateHandlers, obj, actor);
        }
        void onUseItem(const GObject& obj, const GObject& actor, bool force)
        {
            callEngineHandlers(mOnUseItemHandlers, obj, actor, force);
        }
        void onNewExterior(const GCell& cell) { callEngineHandlers(mOnNewExteriorHandlers, cell); }

    private:
        EngineHandlerList mObjectActiveHandlers{ "onObjectActive" };
        EngineHandlerList mActorActiveHandlers{ "onActorActive" };
        EngineHandlerList mItemActiveHandlers{ "onItemActive" };
        EngineHandlerList mNewGameHandlers{ "onNewGame" };
        EngineHandlerList mPlayerAddedHandlers{ "onPlayerAdded" };
        EngineHandlerList mOnActivateHandlers{ "onActivate" };
        EngineHandlerList mOnUseItemHandlers{ "_onUseItem" };
        EngineHandlerList mOnNewExteriorHandlers{ "onNewExterior" };
    };

}

#endif // MWLUA_GLOBALSCRIPTS_H
