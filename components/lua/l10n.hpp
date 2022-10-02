#ifndef COMPONENTS_LUA_L10N_H
#define COMPONENTS_LUA_L10N_H

#include <sol/sol.hpp>

namespace l10n
{
    class Manager;
}

namespace LuaUtil
{
    sol::function initL10nLoader(sol::state& lua, l10n::Manager* manager);
}

#endif // COMPONENTS_LUA_L10N_H
