#ifndef MWLUA_PLAYERSCRIPTS_H
#define MWLUA_PLAYERSCRIPTS_H

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

        void keyPress(int sym, int mod) { callEngineHandlers(mKeyPressHandlers, sym, mod); }

    private:
        EngineHandlerList mKeyPressHandlers{"onKeyPress"};
    };

}

#endif // MWLUA_PLAYERSCRIPTS_H
