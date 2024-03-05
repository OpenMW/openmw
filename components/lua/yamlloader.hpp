#ifndef COMPONENTS_LUA_YAMLLOADER_H
#define COMPONENTS_LUA_YAMLLOADER_H

#include <map>
#include <sol/sol.hpp>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace LuaUtil
{

    class YamlLoader
    {
    public:
        static sol::object load(const std::string& input, const sol::state_view& lua);

        static sol::object load(std::istream& input, const sol::state_view& lua);

    private:
        enum class ScalarType
        {
            Boolean,
            Decimal,
            Float,
            Hexadecimal,
            Infinity,
            NotNumber,
            Null,
            Octal,
            String
        };

        static sol::object load(const std::vector<YAML::Node> rootNodes, const sol::state_view& lua);

        static sol::object getNode(const YAML::Node& node, const sol::state_view& lua, uint64_t depth);

        static sol::table getMap(const YAML::Node& node, const sol::state_view& lua, uint64_t depth);

        static sol::table getArray(const YAML::Node& node, const sol::state_view& lua, uint64_t depth);

        static ScalarType getScalarType(const YAML::Node& node);

        static sol::object getScalar(const YAML::Node& node, const sol::state_view& lua);

        [[noreturn]] static void nodeError(const YAML::Node& node, const std::string& message);
    };

}

#endif // COMPONENTS_LUA_YAMLLOADER_H
