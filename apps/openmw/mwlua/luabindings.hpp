#ifndef MWLUA_LUABINDINGS_H
#define MWLUA_LUABINDINGS_H

#include <map>
#include <sol/forward.hpp>
#include <string>

#include "context.hpp"

namespace MWLua
{
    // Initialize Lua packages that are available for all scripts.
    std::map<std::string, sol::object> initCommonPackages(const Context&);

    // Initialize Lua packages that are available only for global scripts.
    std::map<std::string, sol::object> initGlobalPackages(const Context&);

    // Initialize Lua packages that are available only for local scripts (including player scripts).
    std::map<std::string, sol::object> initLocalPackages(const Context&);

    // Initialize Lua packages that are available only for local scripts on the player.
    std::map<std::string, sol::object> initPlayerPackages(const Context&);
}

#endif // MWLUA_LUABINDINGS_H
