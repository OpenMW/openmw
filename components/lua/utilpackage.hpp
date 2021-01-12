#ifndef COMPONENTS_LUA_UTILPACKAGE_H
#define COMPONENTS_LUA_UTILPACKAGE_H

#include <sol/sol.hpp>

namespace LuaUtil
{

    sol::table initUtilPackage(sol::state&);

}

#endif // COMPONENTS_LUA_UTILPACKAGE_H
