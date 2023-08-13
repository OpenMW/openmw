#ifndef COMPONENTS_LUA_UTF8_H
#define COMPONENTS_LUA_UTF8_H

#include <sol/sol.hpp>

namespace LuaUtf8
{
    sol::table initUtf8Package(sol::state_view&);
}

#endif
