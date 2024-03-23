#include <gtest/gtest.h>

#include <sol/object.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>

#include <yaml-cpp/yaml.h>

#include <components/lua/yamlloader.hpp>

namespace
{
    template <typename T>
    bool checkNumber(sol::state_view& lua, const std::string& inputData, T requiredValue)
    {
        sol::object result = LuaUtil::loadYaml(inputData, lua);
        if (result.get_type() != sol::type::number)
            return false;

        return result.as<T>() == requiredValue;
    }

    bool checkBool(sol::state_view& lua, const std::string& inputData, bool requiredValue)
    {
        sol::object result = LuaUtil::loadYaml(inputData, lua);
        if (result.get_type() != sol::type::boolean)
            return false;

        return result.as<bool>() == requiredValue;
    }

    bool checkNil(sol::state_view& lua, const std::string& inputData)
    {
        sol::object result = LuaUtil::loadYaml(inputData, lua);
        return result == sol::nil;
    }

    bool checkNan(sol::state_view& lua, const std::string& inputData)
    {
        sol::object result = LuaUtil::loadYaml(inputData, lua);
        if (result.get_type() != sol::type::number)
            return false;

        return std::isnan(result.as<double>());
    }

    bool checkString(sol::state_view& lua, const std::string& inputData, const std::string& requiredValue)
    {
        sol::object result = LuaUtil::loadYaml(inputData, lua);
        if (result.get_type() != sol::type::string)
            return false;

        return result.as<std::string>() == requiredValue;
    }

    bool checkString(sol::state_view& lua, const std::string& inputData)
    {
        sol::object result = LuaUtil::loadYaml(inputData, lua);
        if (result.get_type() != sol::type::string)
            return false;

        return result.as<std::string>() == inputData;
    }

    TEST(LuaUtilYamlLoader, ScalarTypeDeduction)
    {
        sol::state lua;

        ASSERT_TRUE(checkNil(lua, "null"));
        ASSERT_TRUE(checkNil(lua, "Null"));
        ASSERT_TRUE(checkNil(lua, "NULL"));
        ASSERT_TRUE(checkNil(lua, "~"));
        ASSERT_TRUE(checkNil(lua, ""));
        ASSERT_FALSE(checkNil(lua, "NUll"));
        ASSERT_TRUE(checkString(lua, "NUll"));
        ASSERT_TRUE(checkString(lua, "'null'", "null"));

        ASSERT_TRUE(checkNumber(lua, "017", 17));
        ASSERT_TRUE(checkNumber(lua, "-017", -17));
        ASSERT_TRUE(checkNumber(lua, "+017", 17));
        ASSERT_TRUE(checkNumber(lua, "17", 17));
        ASSERT_TRUE(checkNumber(lua, "-17", -17));
        ASSERT_TRUE(checkNumber(lua, "+17", 17));
        ASSERT_TRUE(checkNumber(lua, "0o17", 15));
        ASSERT_TRUE(checkString(lua, "-0o17"));
        ASSERT_TRUE(checkString(lua, "+0o17"));
        ASSERT_TRUE(checkString(lua, "0b1"));
        ASSERT_TRUE(checkString(lua, "1:00"));
        ASSERT_TRUE(checkString(lua, "'17'", "17"));
        ASSERT_TRUE(checkNumber(lua, "0x17", 23));
        ASSERT_TRUE(checkString(lua, "'-0x17'", "-0x17"));
        ASSERT_TRUE(checkString(lua, "'+0x17'", "+0x17"));

        ASSERT_TRUE(checkNumber(lua, "2.1e-05", 2.1e-5));
        ASSERT_TRUE(checkNumber(lua, "-2.1e-05", -2.1e-5));
        ASSERT_TRUE(checkNumber(lua, "+2.1e-05", 2.1e-5));
        ASSERT_TRUE(checkNumber(lua, "2.1e+5", 210000));
        ASSERT_TRUE(checkNumber(lua, "-2.1e+5", -210000));
        ASSERT_TRUE(checkNumber(lua, "+2.1e+5", 210000));
        ASSERT_TRUE(checkNumber(lua, "0.27", 0.27));
        ASSERT_TRUE(checkNumber(lua, "-0.27", -0.27));
        ASSERT_TRUE(checkNumber(lua, "+0.27", 0.27));
        ASSERT_TRUE(checkNumber(lua, "2.7", 2.7));
        ASSERT_TRUE(checkNumber(lua, "-2.7", -2.7));
        ASSERT_TRUE(checkNumber(lua, "+2.7", 2.7));
        ASSERT_TRUE(checkNumber(lua, ".27", 0.27));
        ASSERT_TRUE(checkNumber(lua, "-.27", -0.27));
        ASSERT_TRUE(checkNumber(lua, "+.27", 0.27));
        ASSERT_TRUE(checkNumber(lua, "27.", 27.0));
        ASSERT_TRUE(checkNumber(lua, "-27.", -27.0));
        ASSERT_TRUE(checkNumber(lua, "+27.", 27.0));

        ASSERT_TRUE(checkNan(lua, ".nan"));
        ASSERT_TRUE(checkNan(lua, ".NaN"));
        ASSERT_TRUE(checkNan(lua, ".NAN"));
        ASSERT_FALSE(checkNan(lua, "nan"));
        ASSERT_FALSE(checkNan(lua, ".nAn"));
        ASSERT_TRUE(checkString(lua, "'.nan'", ".nan"));
        ASSERT_TRUE(checkString(lua, ".nAn"));

        ASSERT_TRUE(checkNumber(lua, "1.7976931348623157E+308", std::numeric_limits<double>::max()));
        ASSERT_TRUE(checkNumber(lua, "-1.7976931348623157E+308", std::numeric_limits<double>::lowest()));
        ASSERT_TRUE(checkNumber(lua, "2.2250738585072014e-308", std::numeric_limits<double>::min()));
        ASSERT_TRUE(checkNumber(lua, ".inf", std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, "+.inf", std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, "-.inf", -std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, ".Inf", std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, "+.Inf", std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, "-.Inf", -std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, ".INF", std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, "+.INF", std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkNumber(lua, "-.INF", -std::numeric_limits<double>::infinity()));
        ASSERT_TRUE(checkString(lua, ".INf"));
        ASSERT_TRUE(checkString(lua, "-.INf"));
        ASSERT_TRUE(checkString(lua, "+.INf"));

        ASSERT_TRUE(checkBool(lua, "true", true));
        ASSERT_TRUE(checkBool(lua, "false", false));
        ASSERT_TRUE(checkBool(lua, "True", true));
        ASSERT_TRUE(checkBool(lua, "False", false));
        ASSERT_TRUE(checkBool(lua, "TRUE", true));
        ASSERT_TRUE(checkBool(lua, "FALSE", false));
        ASSERT_TRUE(checkString(lua, "y"));
        ASSERT_TRUE(checkString(lua, "n"));
        ASSERT_TRUE(checkString(lua, "On"));
        ASSERT_TRUE(checkString(lua, "Off"));
        ASSERT_TRUE(checkString(lua, "YES"));
        ASSERT_TRUE(checkString(lua, "NO"));
        ASSERT_TRUE(checkString(lua, "TrUe"));
        ASSERT_TRUE(checkString(lua, "FaLsE"));
        ASSERT_TRUE(checkString(lua, "'true'", "true"));
    }

    TEST(LuaUtilYamlLoader, DepthLimit)
    {
        sol::state lua;

        const std::string input = R"(
            array1: &array1_alias
              [
                 <: *array1_alias,
                 foo
              ]
            )";

        bool depthExceptionThrown = false;
        try
        {
            YAML::Node root = YAML::Load(input);
            sol::object result = LuaUtil::loadYaml(input, lua);
        }
        catch (const std::runtime_error& e)
        {
            ASSERT_EQ(std::string(e.what()), "Maximum layers depth exceeded, probably caused by a circular reference");
            depthExceptionThrown = true;
        }

        ASSERT_TRUE(depthExceptionThrown);
    }

    TEST(LuaUtilYamlLoader, Collections)
    {
        sol::state lua;

        sol::object map = LuaUtil::loadYaml("{ x: , y: 2, 4: 5 }", lua);
        ASSERT_EQ(map.as<sol::table>()["x"], sol::nil);
        ASSERT_EQ(map.as<sol::table>()["y"], 2);
        ASSERT_EQ(map.as<sol::table>()[4], 5);

        sol::object array = LuaUtil::loadYaml("[ 3, 4 ]", lua);
        ASSERT_EQ(array.as<sol::table>()[1], 3);

        sol::object emptyTable = LuaUtil::loadYaml("{}", lua);
        ASSERT_TRUE(emptyTable.as<sol::table>().empty());

        sol::object emptyArray = LuaUtil::loadYaml("[]", lua);
        ASSERT_TRUE(emptyArray.as<sol::table>().empty());

        ASSERT_THROW(LuaUtil::loadYaml("{ null: 1 }", lua), std::runtime_error);
        ASSERT_THROW(LuaUtil::loadYaml("{ .nan: 1 }", lua), std::runtime_error);

        const std::string scalarArrayInput = R"(
            - First Scalar
            - 1
            - true)";

        sol::object scalarArray = LuaUtil::loadYaml(scalarArrayInput, lua);
        ASSERT_EQ(scalarArray.as<sol::table>()[1], std::string("First Scalar"));
        ASSERT_EQ(scalarArray.as<sol::table>()[2], 1);
        ASSERT_EQ(scalarArray.as<sol::table>()[3], true);

        const std::string scalarMapWithCommentsInput = R"(
            string: 'str' # String value
            integer: 65 # Integer value
            float: 0.278 # Float value
            bool: false # Boolean value)";

        sol::object scalarMapWithComments = LuaUtil::loadYaml(scalarMapWithCommentsInput, lua);
        ASSERT_EQ(scalarMapWithComments.as<sol::table>()["string"], std::string("str"));
        ASSERT_EQ(scalarMapWithComments.as<sol::table>()["integer"], 65);
        ASSERT_EQ(scalarMapWithComments.as<sol::table>()["float"], 0.278);
        ASSERT_EQ(scalarMapWithComments.as<sol::table>()["bool"], false);

        const std::string mapOfArraysInput = R"(
            x:
            - 2
            - 7
            - true
            y:
            - aaa
            - false
            - 1)";

        sol::object mapOfArrays = LuaUtil::loadYaml(mapOfArraysInput, lua);
        ASSERT_EQ(mapOfArrays.as<sol::table>()["x"][3], true);
        ASSERT_EQ(mapOfArrays.as<sol::table>()["y"][1], std::string("aaa"));

        const std::string arrayOfMapsInput = R"(
            -
              name: Name1
              hr:   65
              avg:  0.278
            -
              name: Name2
              hr:   63
              avg:  0.288)";

        sol::object arrayOfMaps = LuaUtil::loadYaml(arrayOfMapsInput, lua);
        ASSERT_EQ(arrayOfMaps.as<sol::table>()[1]["avg"], 0.278);
        ASSERT_EQ(arrayOfMaps.as<sol::table>()[2]["name"], std::string("Name2"));

        const std::string arrayOfArraysInput = R"(
            - [Name1, 65, 0.278]
            - [Name2  , 63, 0.288])";

        sol::object arrayOfArrays = LuaUtil::loadYaml(arrayOfArraysInput, lua);
        ASSERT_EQ(arrayOfArrays.as<sol::table>()[1][2], 65);
        ASSERT_EQ(arrayOfArrays.as<sol::table>()[2][1], std::string("Name2"));

        const std::string mapOfMapsInput = R"(
            Name1: {hr: 65, avg: 0.278}
            Name2 : {
                hr: 63,
                avg: 0.288,
             })";

        sol::object mapOfMaps = LuaUtil::loadYaml(mapOfMapsInput, lua);
        ASSERT_EQ(mapOfMaps.as<sol::table>()["Name1"]["hr"], 65);
        ASSERT_EQ(mapOfMaps.as<sol::table>()["Name2"]["avg"], 0.288);
    }

    TEST(LuaUtilYamlLoader, Structures)
    {
        sol::state lua;

        const std::string twoDocumentsInput
            = "---\n"
              "    - First Scalar\n"
              "    - 2\n"
              "    - true\n"
              "\n"
              "---\n"
              "    - Second Scalar\n"
              "    - 3\n"
              "    - false";

        sol::object twoDocuments = LuaUtil::loadYaml(twoDocumentsInput, lua);
        ASSERT_EQ(twoDocuments.as<sol::table>()[1][1], std::string("First Scalar"));
        ASSERT_EQ(twoDocuments.as<sol::table>()[2][3], false);

        const std::string anchorInput = R"(---
            x:
            - Name1
            # Following node labeled as "a"
            - &a Value1
            y:
            - *a # Subsequent occurrence
            - Name2)";

        sol::object anchor = LuaUtil::loadYaml(anchorInput, lua);
        ASSERT_EQ(anchor.as<sol::table>()["y"][1], std::string("Value1"));

        const std::string compoundKeyInput = R"(
            ? - String1
              - String2
            : - 1

            ? [ String3,
                String4 ]
            : [ 2, 3, 4 ])";

        ASSERT_THROW(LuaUtil::loadYaml(compoundKeyInput, lua), std::runtime_error);

        const std::string compactNestedMappingInput = R"(
            - item    : Item1
              quantity: 2
            - item    : Item2
              quantity: 4
            - item    : Item3
              quantity: 11)";

        sol::object compactNestedMapping = LuaUtil::loadYaml(compactNestedMappingInput, lua);
        ASSERT_EQ(compactNestedMapping.as<sol::table>()[2]["quantity"], 4);
    }

    TEST(LuaUtilYamlLoader, Scalars)
    {
        sol::state lua;

        const std::string literalScalarInput = R"(--- |
              a
              b
              c)";

        ASSERT_TRUE(checkString(lua, literalScalarInput, "a\nb\nc"));

        const std::string foldedScalarInput = R"(--- >
              a
              b
              c)";

        ASSERT_TRUE(checkString(lua, foldedScalarInput, "a b c"));

        const std::string multiLinePlanarScalarsInput = R"(
            plain:
              This unquoted scalar
              spans many lines.

            quoted: "So does this
              quoted scalar.\n")";

        sol::object multiLinePlanarScalars = LuaUtil::loadYaml(multiLinePlanarScalarsInput, lua);
        ASSERT_TRUE(
            multiLinePlanarScalars.as<sol::table>()["plain"] == std::string("This unquoted scalar spans many lines."));
        ASSERT_TRUE(multiLinePlanarScalars.as<sol::table>()["quoted"] == std::string("So does this quoted scalar.\n"));
    }
}
