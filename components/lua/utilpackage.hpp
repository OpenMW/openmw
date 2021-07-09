#ifndef COMPONENTS_LUA_UTILPACKAGE_H
#define COMPONENTS_LUA_UTILPACKAGE_H

#include <limits> // missing from sol/sol.hpp
#include <sol/sol.hpp>

namespace LuaUtil
{

    sol::table initUtilPackage(sol::state&);

}

#endif // COMPONENTS_LUA_UTILPACKAGE_H
