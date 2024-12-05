#include "apps/openmw/options.hpp"

#include <components/files/configurationmanager.hpp>
#include <components/files/conversion.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using namespace testing;
    using namespace OpenMW;

    namespace bpo = boost::program_options;

    template <class T, std::size_t size>
    std::string makeString(const T (&range)[size])
    {
        static_assert(size > 0);
        return std::string(std::begin(range), std::end(range) - 1);
    }

    template <class... Args>
    std::vector<std::string> generateSupportedCharacters(Args&&... args)
    {
        std::vector<std::string> result;
        (result.emplace_back(makeString(args)), ...);
        for (int i = 1; i <= std::numeric_limits<char>::max(); ++i)
            if (i != '&' && i != '"' && i != ' ' && i != '\n')
                result.push_back(std::string(1, static_cast<char>(i)));
        return result;
    }

    MATCHER_P(IsPath, v, "")
    {
        return Files::pathToUnicodeString(arg) == v;
    }

    template <class T>
    void parseArgs(const T& arguments, bpo::variables_map& variables, bpo::options_description& description)
    {
        Files::parseArgs(static_cast<int>(arguments.size()), arguments.data(), variables, description);
    }

    TEST(OpenMWOptionsFromArguments, should_support_equality_to_separate_flag_and_value)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame=save.omwsave" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_single_word_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", "save.omwsave" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_multi_component_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", "/home/user/openmw/save.omwsave" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()),
            "/home/user/openmw/save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_windows_multi_component_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", R"(C:\OpenMW\save.omwsave)" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()),
            R"(C:\OpenMW\save.omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_spaces)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", "my save.omwsave" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "my save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_octothorpe)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", "my#save.omwsave" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "my#save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_at_sign)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", "my@save.omwsave" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "my@save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_quote)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", R"(my"save.omwsave)" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(my"save.omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_quoted_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", R"("save".omwsave)" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(save)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_quoted_load_savegame_path_with_escaped_quote_by_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", R"("save&".omwsave")" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(save".omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_quoted_load_savegame_path_with_escaped_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", R"("save.omwsave&&")" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save.omwsave&");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", "save&.omwsave" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save&.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_multiple_quotes)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", R"(my"save".omwsave)" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(my"save".omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_compose_data)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--data", "1", "--data", "2" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_THAT(variables["data"].as<Files::MaybeQuotedPathContainer>(), ElementsAre(IsPath("1"), IsPath("2")));
    }

    TEST(OpenMWOptionsFromArguments, should_compose_data_from_single_flag)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--data", "1", "2" };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_THAT(variables["data"].as<Files::MaybeQuotedPathContainer>(), ElementsAre(IsPath("1"), IsPath("2")));
    }

    TEST(OpenMWOptionsFromArguments, should_throw_on_multiple_load_savegame)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments{ "openmw", "--load-savegame", "1.omwsave", "--load-savegame", "2.omwsave" };
        bpo::variables_map variables;
        EXPECT_THROW(parseArgs(arguments, variables, description), std::exception);
    }

    struct OpenMWOptionsFromArgumentsStrings : TestWithParam<std::string>
    {
    };

    TEST_P(OpenMWOptionsFromArgumentsStrings, should_support_paths_with_certain_characters_in_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::string path = "save_" + std::string(GetParam()) + ".omwsave";
        const std::string pathArgument = "\"" + path + "\"";
        const std::array arguments{ "openmw", "--load-savegame", pathArgument.c_str() };
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), path);
    }

    INSTANTIATE_TEST_SUITE_P(SupportedCharacters, OpenMWOptionsFromArgumentsStrings,
        ValuesIn(generateSupportedCharacters(u8"👍", u8"Ъ", u8"Ǽ", "\n")));

    TEST(OpenMWOptionsFromConfig, should_support_single_word_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_strip_quotes_from_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="save.omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_strip_outer_quotes_from_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame=""save".omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "");
    }

    TEST(OpenMWOptionsFromConfig, should_strip_quotes_from_load_savegame_path_with_space)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="my save.omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "my save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_octothorpe)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save#.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save#.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_at_sign)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save@.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save@.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_quote)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame=save".omwsave)");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(save".omwsave)");
    }

    TEST(OpenMWOptionsFromConfig, should_support_confusing_savegame_path_with_lots_going_on)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="one &"two"three".omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(one "two)");
    }

    TEST(OpenMWOptionsFromConfig, should_support_confusing_savegame_path_with_even_more_going_on)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="one &"two"three ".omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(one "two)");
    }

    TEST(OpenMWOptionsFromConfig, should_ignore_commented_option)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("#load-savegame=save.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "");
    }

    TEST(OpenMWOptionsFromConfig, should_ignore_whitespace_prefixed_commented_option)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(" \t#load-savegame=save.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "");
    }

    TEST(OpenMWOptionsFromConfig, should_support_whitespace_around_option)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(" load-savegame = save.omwsave ");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_throw_on_multiple_load_savegame)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=1.omwsave\nload-savegame=2.omwsave");
        bpo::variables_map variables;
        EXPECT_THROW(Files::parseConfig(stream, variables, description), std::exception);
    }

    TEST(OpenMWOptionsFromConfig, should_support_multi_component_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=/home/user/openmw/save.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()),
            "/home/user/openmw/save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_windows_multi_component_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame=C:\OpenMW\save.omwsave)");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()),
            R"(C:\OpenMW\save.omwsave)");
    }

    TEST(OpenMWOptionsFromConfig, should_compose_data)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("data=1\ndata=2");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_THAT(variables["data"].as<Files::MaybeQuotedPathContainer>(), ElementsAre(IsPath("1"), IsPath("2")));
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_escaped_quote_by_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="save&".omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(
            Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), R"(save".omwsave)");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_escaped_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="save.omwsave&&")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save.omwsave&");
    }

    TEST(OpenMWOptionsFromConfig, should_support_load_savegame_path_with_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save&.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), "save&.omwsave");
    }

    struct OpenMWOptionsFromConfigStrings : TestWithParam<std::string>
    {
    };

    TEST_P(OpenMWOptionsFromConfigStrings, should_support_paths_with_certain_characters_in_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::string path = "save_" + std::string(GetParam()) + ".omwsave";
        std::istringstream stream("load-savegame=\"" + path + "\"");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(Files::pathToUnicodeString(variables["load-savegame"].as<Files::MaybeQuotedPath>()), path);
    }

    INSTANTIATE_TEST_SUITE_P(SupportedCharacters, OpenMWOptionsFromConfigStrings,
        ValuesIn(generateSupportedCharacters(u8"👍", u8"Ъ", u8"Ǽ")));
}
