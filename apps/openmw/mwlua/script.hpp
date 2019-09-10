#ifndef GAME_MWLUA_SCRIPT_H
#define GAME_MWLUA_SCRIPT_H

#include <extern/sol2/sol.hpp>

#include "dynamicluaobject.hpp"

#include <components/esm/loadscpt.hpp>

#include "../mwworld/ptr.hpp"

namespace MWLua
{
    struct Script : public DynamicLuaObject
    {
        Script() :
            mScript(nullptr),
            mReference(MWWorld::Ptr())
        {
        }
        ESM::Script* mScript;
        MWWorld::Ptr mReference;
    };
}

#endif
