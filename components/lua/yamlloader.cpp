#include "yamlloader.hpp"

#include <charconv>
#include <cmath>
#include <limits>
#include <regex>
#include <stdexcept>
#include <system_error>
#include <vector>

#include <sol/object.hpp>
#include <sol/state_view.hpp>

#include <yaml-cpp/yaml.h>

#include <components/misc/strings/format.hpp>
#include <components/misc/strings/lower.hpp>

namespace LuaUtil
{
    namespace
    {
        constexpr uint64_t maxDepth = 250;

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

        sol::object loadAll(const std::vector<YAML::Node>& rootNodes, const sol::state_view& lua);

        sol::object getNode(const YAML::Node& node, const sol::state_view& lua, uint64_t depth);

        sol::table getMap(const YAML::Node& node, const sol::state_view& lua, uint64_t depth);

        sol::table getArray(const YAML::Node& node, const sol::state_view& lua, uint64_t depth);

        ScalarType getScalarType(const YAML::Node& node);

        sol::object getScalar(const YAML::Node& node, const sol::state_view& lua);

        [[noreturn]] void nodeError(const YAML::Node& node, const std::string& message);
    }

    sol::object loadYaml(const std::string& input, const sol::state_view& lua)
    {
        std::vector<YAML::Node> rootNodes = YAML::LoadAll(input);
        return loadAll(rootNodes, lua);
    }

    sol::object loadYaml(std::istream& input, const sol::state_view& lua)
    {
        std::vector<YAML::Node> rootNodes = YAML::LoadAll(input);
        return loadAll(rootNodes, lua);
    }

    namespace
    {
        sol::object loadAll(const std::vector<YAML::Node>& rootNodes, const sol::state_view& lua)
        {
            if (rootNodes.empty())
                return sol::nil;

            if (rootNodes.size() == 1)
                return getNode(rootNodes[0], lua, 0);

            sol::table documentsTable(lua, sol::create);
            for (const auto& root : rootNodes)
            {
                documentsTable.add(getNode(root, lua, 1));
            }

            return documentsTable;
        }

        sol::object getNode(const YAML::Node& node, const sol::state_view& lua, uint64_t depth)
        {
            if (depth >= maxDepth)
                throw std::runtime_error("Maximum layers depth exceeded, probably caused by a circular reference");

            ++depth;

            if (node.IsMap())
                return getMap(node, lua, depth);
            else if (node.IsSequence())
                return getArray(node, lua, depth);
            else if (node.IsScalar())
                return getScalar(node, lua);
            else if (node.IsNull())
                return sol::nil;

            nodeError(node, "An unknown YAML node encountered");
        }

        sol::table getMap(const YAML::Node& node, const sol::state_view& lua, uint64_t depth)
        {
            sol::table childTable(lua, sol::create);

            for (const auto& pair : node)
            {
                if (pair.first.IsMap())
                    nodeError(pair.first, "Only scalar nodes can be used as keys, encountered map instead");
                if (pair.first.IsSequence())
                    nodeError(pair.first, "Only scalar nodes can be used as keys, encountered array instead");
                if (pair.first.IsNull())
                    nodeError(pair.first, "Only scalar nodes can be used as keys, encountered null instead");

                auto key = getNode(pair.first, lua, depth);
                if (key.get_type() == sol::type::number && std::isnan(key.as<double>()))
                    nodeError(pair.first, "Only scalar nodes can be used as keys, encountered nan instead");

                childTable[key] = getNode(pair.second, lua, depth);
            }

            return childTable;
        }

        sol::table getArray(const YAML::Node& node, const sol::state_view& lua, uint64_t depth)
        {
            sol::table childTable(lua, sol::create);

            for (const auto& child : node)
            {
                childTable.add(getNode(child, lua, depth));
            }

            return childTable;
        }

        ScalarType getScalarType(const YAML::Node& node)
        {
            const auto& tag = node.Tag();
            const auto& value = node.Scalar();
            if (tag == "!")
                return ScalarType::String;

            // Note that YAML allows to explicitely specify a scalar type via tag (e.g. "!!bool"), but it makes no
            // sense in Lua:
            // 1. Both integers and floats use the "number" type prior to Lua 5.3
            // 2. Strings can be quoted, which is more readable than "!!str"
            // 3. Most of possible conversions are invalid or their result is unclear
            // So ignore this feature for now.
            if (tag != "?")
                nodeError(node, "An invalid tag '" + tag + "' encountered");

            if (value.empty())
                return ScalarType::Null;

            // Resolve type according to YAML 1.2 Core Schema (see https://yaml.org/spec/1.2.2/#103-core-schema)
            static const std::regex boolRegex("true|True|TRUE|false|False|FALSE", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), boolRegex))
                return ScalarType::Boolean;

            static const std::regex decimalRegex("[-+]?[0-9]+", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), decimalRegex))
                return ScalarType::Decimal;

            static const std::regex floatRegex(
                "[-+]?([.][0-9]+|[0-9]+([.][0-9]*)?)([eE][-+]?[0-9]+)?", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), floatRegex))
                return ScalarType::Float;

            static const std::regex octalRegex("0o[0-7]+", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), octalRegex))
                return ScalarType::Octal;

            static const std::regex hexdecimalRegex("0x[0-9a-fA-F]+", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), hexdecimalRegex))
                return ScalarType::Hexadecimal;

            static const std::regex infinityRegex("[-+]?([.]inf|[.]Inf|[.]INF)", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), infinityRegex))
                return ScalarType::Infinity;

            static const std::regex nanRegex("[.]nan|[.]NaN|[.]NAN", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), nanRegex))
                return ScalarType::NotNumber;

            static const std::regex nullRegex("null|Null|NULL|~", std::regex_constants::extended);
            if (std::regex_match(node.Scalar(), nullRegex))
                return ScalarType::Null;

            return ScalarType::String;
        }

        sol::object getScalar(const YAML::Node& node, const sol::state_view& lua)
        {
            auto type = getScalarType(node);
            const auto& value = node.Scalar();

            switch (type)
            {
                case ScalarType::Null:
                    return sol::nil;
                case ScalarType::String:
                    return sol::make_object<std::string>(lua, value);
                case ScalarType::NotNumber:
                    return sol::make_object<double>(lua, std::nan(""));
                case ScalarType::Infinity:
                {
                    if (!value.empty() && value[0] == '-')
                        return sol::make_object<double>(lua, -std::numeric_limits<double>::infinity());

                    return sol::make_object<double>(lua, std::numeric_limits<double>::infinity());
                }
                case ScalarType::Boolean:
                {
                    if (Misc::StringUtils::lowerCase(value) == "true")
                        return sol::make_object<bool>(lua, true);

                    if (Misc::StringUtils::lowerCase(value) == "false")
                        return sol::make_object<bool>(lua, false);

                    nodeError(node, "Can not read a boolean value '" + value + "'");
                }
                case ScalarType::Decimal:
                {
                    int offset = 0;

                    // std::from_chars does not support "+" sign
                    if (!value.empty() && value[0] == '+')
                        ++offset;

                    int result = 0;
                    const auto status = std::from_chars(value.data() + offset, value.data() + value.size(), result);
                    if (status.ec == std::errc())
                        return sol::make_object<int>(lua, result);

                    nodeError(node, "Can not read a decimal value '" + value + "'");
                }
                case ScalarType::Float:
                {
                    // Not all compilers support std::from_chars for floats
                    double result = 0.0;
                    bool success = YAML::convert<double>::decode(node, result);
                    if (success)
                        return sol::make_object<double>(lua, result);

                    nodeError(node, "Can not read a float value '" + value + "'");
                }
                case ScalarType::Hexadecimal:
                {
                    int result = 0;
                    const auto status = std::from_chars(value.data() + 2, value.data() + value.size(), result, 16);
                    if (status.ec == std::errc())
                        return sol::make_object<int>(lua, result);

                    nodeError(node, "Can not read a hexadecimal value '" + value + "'");
                }
                case ScalarType::Octal:
                {
                    int result = 0;
                    const auto status = std::from_chars(value.data() + 2, value.data() + value.size(), result, 8);
                    if (status.ec == std::errc())
                        return sol::make_object<int>(lua, result);

                    nodeError(node, "Can not read an octal value '" + value + "'");
                }
                default:
                    nodeError(node, "An unknown scalar '" + value + "' encountered");
            }
        }

        [[noreturn]] void nodeError(const YAML::Node& node, const std::string& message)
        {
            const auto& mark = node.Mark();
            std::string error = Misc::StringUtils::format(
                " at line=%d column=%d position=%d", mark.line + 1, mark.column + 1, mark.pos + 1);
            throw std::runtime_error(message + error);
        }
    }
}
