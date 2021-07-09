#ifndef MWLUA_PLAYERSCRIPTS_H
#define MWLUA_PLAYERSCRIPTS_H

#include <SDL_events.h>

#include "localscripts.hpp"

namespace MWLua
{

    class PlayerScripts : public LocalScripts
    {
    public:
        PlayerScripts(LuaUtil::LuaState* lua, const LObject& obj) : LocalScripts(lua, obj)
        {
            registerEngineHandlers({&mKeyPressHandlers});
        }

        void keyPress(const SDL_Keysym& key) { callEngineHandlers(mKeyPressHandlers, key); }

    private:
        EngineHandlerList mKeyPressHandlers{"onKeyPress"};
    };

}

#endif // MWLUA_PLAYERSCRIPTS_H
