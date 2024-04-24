#ifndef COMPONENTS_LUA_YAMLLOADER_H
#define COMPONENTS_LUA_YAMLLOADER_H

#include <iosfwd>
#include <string>

#include <sol/forward.hpp>

namespace LuaUtil
{
    sol::object loadYaml(const std::string& input, const sol::state_view& lua);

    sol::object loadYaml(std::istream& input, const sol::state_view& lua);
}

#endif // COMPONENTS_LUA_YAMLLOADER_H
