#ifndef GAME_MWLUA_SCRIPT_H
#define GAME_MWLUA_SCRIPT_H

#include <extern/sol2/sol.hpp>

#include "dynamicluaobject.hpp"

#include <components/esm/loadscpt.hpp>

#include "../mwworld/ptr.hpp"

namespace MWLua
{
    struct LuaScript : public DynamicLuaObject
    {
        LuaScript() :
            script(nullptr),
            reference(MWWorld::Ptr())
        {
        }
        ESM::Script* script;
        MWWorld::Ptr reference;
    };
}

#endif
