#include <apps/openmw/options.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/files/escape.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using namespace testing;
    using namespace OpenMW;

    namespace bpo = boost::program_options;

    std::vector<std::string> generateSupportedCharacters(std::vector<std::string>&& base = {})
    {
        std::vector<std::string> result = std::move(base);
        for (int i = 1; i <= std::numeric_limits<char>::max(); ++i)
            if (i != '&' && i != '"' && i != ' ' && i != '@' && i != '\n')
                result.push_back(std::string(1, i));
        return result;
    }

    constexpr std::array supportedAtSignEscapings {
        std::pair {'a', '@'},
        std::pair {'h', '#'},
    };

    MATCHER_P(IsEscapePath, v, "") { return arg.mPath.string() == v; }

    template <class T>
    void parseArgs(const T& arguments, bpo::variables_map& variables, bpo::options_description& description)
    {
        Files::parseArgs(static_cast<int>(arguments.size()), arguments.data(), variables, description);
    }

    TEST(OpenMWOptionsFromArguments, should_support_equality_to_separate_flag_and_value)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame=save.omwsave"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_single_word_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", "save.omwsave"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_multi_component_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", "/home/user/openmw/save.omwsave"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "/home/user/openmw/save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_windows_multi_component_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", R"(C:\OpenMW\save.omwsave)"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(C:\OpenMW\save.omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_spaces)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", "my save.omwsave"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "my");
//        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "my save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_number_sign)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", "my#save.omwsave"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "my#save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_at_sign)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", "my@save.omwsave"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "my?ave.omwsave");
//        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "my@save.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_quote)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", R"(my"save.omwsave)"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(my"save.omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_quted_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", R"("save".omwsave)"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(save)");
//        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"("save".omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_quoted_load_savegame_path_with_escaped_quote_by_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", R"("save&".omwsave")"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(save".omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_support_quoted_load_savegame_path_with_escaped_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", R"("save.omwsave&&")"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save.omwsave&");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", "save&.omwsave"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save&.omwsave");
    }

    TEST(OpenMWOptionsFromArguments, should_support_load_savegame_path_with_multiple_quotes)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", R"(my"save".omwsave)"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(my"save".omwsave)");
    }

    TEST(OpenMWOptionsFromArguments, should_compose_data)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--data", "1", "--data", "2"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_THAT(variables["data"].as<Files::EscapePathContainer>(), ElementsAre(IsEscapePath("1"), IsEscapePath("2")));
    }

    TEST(OpenMWOptionsFromArguments, should_compose_data_from_single_flag)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--data", "1", "2"};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_THAT(variables["data"].as<Files::EscapePathContainer>(), ElementsAre(IsEscapePath("1"), IsEscapePath("2")));
    }

    TEST(OpenMWOptionsFromArguments, should_throw_on_multiple_load_savegame)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::array arguments {"openmw", "--load-savegame", "1.omwsave", "--load-savegame", "2.omwsave"};
        bpo::variables_map variables;
        EXPECT_THROW(parseArgs(arguments, variables, description), std::exception);
    }

    struct OpenMWOptionsFromArgumentsStrings : TestWithParam<std::string> {};

    TEST_P(OpenMWOptionsFromArgumentsStrings, should_support_paths_with_certain_characters_in_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::string path = "save_" + std::string(GetParam()) + ".omwsave";
        const std::string pathArgument = "\"" + path + "\"";
        const std::array arguments {"openmw", "--load-savegame", pathArgument.c_str()};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), path);
    }

    INSTANTIATE_TEST_SUITE_P(
        SupportedCharacters,
        OpenMWOptionsFromArgumentsStrings,
        ValuesIn(generateSupportedCharacters({u8"👍", u8"Ъ", u8"Ǽ", "\n"}))
    );

    struct OpenMWOptionsFromArgumentsEscapings : TestWithParam<std::pair<char, char>> {};

    TEST_P(OpenMWOptionsFromArgumentsEscapings, should_support_escaping_with_at_sign_in_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::string path = "save_@" + std::string(1, GetParam().first) + ".omwsave";
        const std::array arguments {"openmw", "--load-savegame", path.c_str()};
        bpo::variables_map variables;
        parseArgs(arguments, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(),
                  "save_" + std::string(1, GetParam().second) + ".omwsave");
    }

    INSTANTIATE_TEST_SUITE_P(
        SupportedEscapingsWithAtSign,
        OpenMWOptionsFromArgumentsEscapings,
        ValuesIn(supportedAtSignEscapings)
    );

    TEST(OpenMWOptionsFromConfig, should_support_single_word_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_strip_quotes_from_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="save.omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_strip_outer_quotes_from_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame=""save".omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "");
//        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(""save".omwsave")");
    }

    TEST(OpenMWOptionsFromConfig, should_strip_quotes_from_load_savegame_path_with_space)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="my save.omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "my save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_number_sign)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save#.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save#.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_at_sign)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save@.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save@.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_quote)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame=save".omwsave)");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(save".omwsave)");
    }

    TEST(OpenMWOptionsFromConfig, should_ignore_commented_option)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("#load-savegame=save.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "");
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
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "/home/user/openmw/save.omwsave");
    }

    TEST(OpenMWOptionsFromConfig, should_support_windows_multi_component_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame=C:\OpenMW\save.omwsave)");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(C:\OpenMW\save.omwsave)");
    }

    TEST(OpenMWOptionsFromConfig, should_compose_data)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("data=1\ndata=2");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_THAT(variables["data"].as<Files::EscapePathContainer>(), ElementsAre(IsEscapePath("1"), IsEscapePath("2")));
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_escaped_quote_by_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="save&".omwsave")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), R"(save".omwsave)");
    }

    TEST(OpenMWOptionsFromConfig, should_support_quoted_load_savegame_path_with_escaped_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream(R"(load-savegame="save.omwsave&&")");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save.omwsave&");
    }

    TEST(OpenMWOptionsFromConfig, should_support_load_savegame_path_with_ampersand)
    {
        bpo::options_description description = makeOptionsDescription();
        std::istringstream stream("load-savegame=save&.omwsave");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), "save&.omwsave");
    }

    struct OpenMWOptionsFromConfigStrings : TestWithParam<std::string> {};

    TEST_P(OpenMWOptionsFromConfigStrings, should_support_paths_with_certain_characters_in_load_savegame_path)
    {
        bpo::options_description description = makeOptionsDescription();
        const std::string path = "save_" + std::string(GetParam()) + ".omwsave";
        std::istringstream stream("load-savegame=\"" + path + "\"");
        bpo::variables_map variables;
        Files::parseConfig(stream, variables, description);
        EXPECT_EQ(variables["load-savegame"].as<Files::EscapePath>().mPath.string(), path);
    }

    INSTANTIATE_TEST_SUITE_P(
        SupportedCharacters,
        OpenMWOptionsFromConfigStrings,
        ValuesIn(generateSupportedCharacters({u8"👍", u8"Ъ", u8"Ǽ"}))
    );
}
