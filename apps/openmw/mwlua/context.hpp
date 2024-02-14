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
    class ObjectLists;

    struct Context
    {
        enum Type
        {
            Menu,
            Global,
            Local,
        };
        Type mType;
        LuaManager* mLuaManager;
        LuaUtil::LuaState* mLua;
        LuaUtil::UserdataSerializer* mSerializer;
        ObjectLists* mObjectLists;
        LuaEvents* mLuaEvents;
    };

}

#endif // MWLUA_CONTEXT_H
