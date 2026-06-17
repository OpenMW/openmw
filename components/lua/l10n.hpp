#ifndef COMPONENTS_LUA_L10N_H
#define COMPONENTS_LUA_L10N_H

#include <sol/sol.hpp>

namespace L10n
{
    class Manager;
}

namespace LuaUtil
{
    sol::function initL10nLoader(lua_State* state, L10n::Manager* manager);
}

#endif // COMPONENTS_LUA_L10N_H
