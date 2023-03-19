#ifndef MWLUA_CONTEXT_H
#define MWLUA_CONTEXT_H

namespace LuaUtil
{
    class LuaState;
    class UserdataSerializer;
}

namespace MWLua
{
    class LuaEvents;
    class LuaManager;
    class WorldView;

    struct Context
    {
        bool mIsGlobal;
        LuaManager* mLuaManager;
        LuaUtil::LuaState* mLua;
        LuaUtil::UserdataSerializer* mSerializer;
        WorldView* mWorldView;
        LuaEvents* mLuaEvents;
    };

}

#endif // MWLUA_CONTEXT_H
