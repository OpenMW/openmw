#ifndef MWLUA_CONTEXT_H
#define MWLUA_CONTEXT_H

#include "eventqueue.hpp"

namespace LuaUtil
{
    class LuaState;
    class UserdataSerializer;
}

namespace MWLua
{
    class LuaManager;
    class WorldView;

    struct Context
    {
        bool mIsGlobal;
        LuaManager* mLuaManager;
        LuaUtil::LuaState* mLua;
        LuaUtil::UserdataSerializer* mSerializer;
        WorldView* mWorldView;
        LocalEventQueue* mLocalEventQueue;
        GlobalEventQueue* mGlobalEventQueue;
    };

}

#endif // MWLUA_CONTEXT_H
