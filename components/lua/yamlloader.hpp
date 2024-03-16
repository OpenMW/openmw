#ifndef COMPONENTS_LUA_YAMLLOADER_H
#define COMPONENTS_LUA_YAMLLOADER_H

#include <sol/sol.hpp>

namespace LuaUtil
{

    namespace YamlLoader
    {
        sol::object load(const std::string& input, const sol::state_view& lua);

        sol::object load(std::istream& input, const sol::state_view& lua);
    }

}

#endif // COMPONENTS_LUA_YAMLLOADER_H
