#ifndef MWLUA_LUABINDINGS_H
#define MWLUA_LUABINDINGS_H

#include <sol/forward.hpp>

#include <map>
#include <string>

namespace MWLua
{
    struct Context;

    // Initialize Lua packages that are available for all scripts.
    std::map<std::string, sol::object> initCommonPackages(const Context& context);

    // Initialize Lua packages that are available for global scripts (additionally to common packages).
    std::map<std::string, sol::object> initGlobalPackages(const Context& context);

    // Initialize Lua packages that are available for local scripts (additionally to common packages).
    std::map<std::string, sol::object> initLocalPackages(const Context& context);

    // Initialize Lua packages that are available only for local scripts on the player (additionally to common and local
    // packages).
    std::map<std::string, sol::object> initPlayerPackages(const Context& context);

    // Initialize Lua packages that are available only for menu scripts (additionally to common packages).
    std::map<std::string, sol::object> initMenuPackages(const Context& context);
}

#endif // MWLUA_LUABINDINGS_H
