#include <components/settings/parser.hpp>

#include <boost/filesystem/fstream.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace Settings;

    struct SettingsFileParserTest : Test
    {
        SettingsFileParser mLoader;
        SettingsFileParser mSaver;

        template <typename F>
        void withSettingsFile( const std::string& content, F&& f)
        {
            const auto path = std::string(UnitTest::GetInstance()->current_test_info()->name()) + ".cfg";

            {
                boost::filesystem::ofstream stream;
                stream.open(path);
                stream << content;
                stream.close();
            }

            f(path);
        }
    };

    TEST_F(SettingsFileParserTest, load_empty_file)
    {
        const std::string content;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap());
        });
    }

    TEST_F(SettingsFileParserTest, file_with_single_empty_section)
    {
        const std::string content =
            "[Section]\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap());
        });
    }

    TEST_F(SettingsFileParserTest, file_with_single_section_and_key)
    {
        const std::string content =
            "[Section]\n"
            "key = value\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), "value"}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_single_section_and_key_and_line_comments)
    {
        const std::string content =
            "# foo\n"
            "[Section]\n"
            "# bar\n"
            "key = value\n"
            "# baz\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), "value"}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_single_section_and_key_file_and_inline_section_comment)
    {
        const std::string content =
            "[Section] # foo\n"
            "key = value\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            EXPECT_THROW(mLoader.loadSettingsFile(path, map), std::runtime_error);
        });
    }

    TEST_F(SettingsFileParserTest, file_single_section_and_key_and_inline_key_comment)
    {
        const std::string content =
            "[Section]\n"
            "key = value # foo\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), "value # foo"}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_single_section_and_key_and_whitespaces)
    {
        const std::string content =
            " [ Section ] \n"
            " key = value \n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), "value"}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_quoted_string_value)
    {
        const std::string content =
            "[Section]\n"
            R"(key = "value")"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), R"("value")"}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_quoted_string_value_and_eol)
    {
        const std::string content =
            "[Section]\n"
            R"(key = "value"\n)"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), R"("value"\n)"}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_empty_value)
    {
        const std::string content =
            "[Section]\n"
            "key =\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), ""}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_empty_key)
    {
        const std::string content =
            "[Section]\n"
            "=\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", ""), ""}
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_multiple_keys)
    {
        const std::string content =
            "[Section]\n"
            "key1 = value1\n"
            "key2 = value2\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key1"), "value1"},
                {CategorySetting("Section", "key2"), "value2"},
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_multiple_sections)
    {
        const std::string content =
            "[Section1]\n"
            "key1 = value1\n"
            "[Section2]\n"
            "key2 = value2\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section1", "key1"), "value1"},
                {CategorySetting("Section2", "key2"), "value2"},
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_multiple_sections_and_keys)
    {
        const std::string content =
            "[Section1]\n"
            "key1 = value1\n"
            "key2 = value2\n"
            "[Section2]\n"
            "key3 = value3\n"
            "key4 = value4\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section1", "key1"), "value1"},
                {CategorySetting("Section1", "key2"), "value2"},
                {CategorySetting("Section2", "key3"), "value3"},
                {CategorySetting("Section2", "key4"), "value4"},
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_repeated_sections)
    {
        const std::string content =
            "[Section]\n"
            "key1 = value1\n"
            "[Section]\n"
            "key2 = value2\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key1"), "value1"},
                {CategorySetting("Section", "key2"), "value2"},
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_repeated_keys)
    {
        const std::string content =
            "[Section]\n"
            "key = value\n"
            "key = value\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            EXPECT_THROW(mLoader.loadSettingsFile(path, map), std::runtime_error);
        });
    }

    TEST_F(SettingsFileParserTest, file_with_repeated_keys_in_differrent_sections)
    {
        const std::string content =
            "[Section1]\n"
            "key = value\n"
            "[Section2]\n"
            "key = value\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section1", "key"), "value"},
                {CategorySetting("Section2", "key"), "value"},
            }));
        });
    }

    TEST_F(SettingsFileParserTest, file_with_unterminated_section)
    {
        const std::string content =
            "[Section"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            EXPECT_THROW(mLoader.loadSettingsFile(path, map), std::runtime_error);
        });
    }

    TEST_F(SettingsFileParserTest, file_with_single_empty_section_name)
    {
        const std::string content =
            "[]\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap());
        });
    }

    TEST_F(SettingsFileParserTest, file_with_key_and_without_section)
    {
        const std::string content =
            "key = value\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            EXPECT_THROW(mLoader.loadSettingsFile(path, map), std::runtime_error);
        });
    }

    TEST_F(SettingsFileParserTest, file_with_key_in_empty_name_section)
    {
        const std::string content =
            "[]"
            "key = value\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            EXPECT_THROW(mLoader.loadSettingsFile(path, map), std::runtime_error);
        });
    }

    TEST_F(SettingsFileParserTest, file_with_unterminated_key)
    {
        const std::string content =
            "[Section]\n"
            "key\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            EXPECT_THROW(mLoader.loadSettingsFile(path, map), std::runtime_error);
        });
    }

    TEST_F(SettingsFileParserTest, file_with_empty_lines)
    {
        const std::string content =
            "\n"
            "[Section]\n"
            "\n"
            "key = value\n"
            "\n"
        ;

        withSettingsFile(content, [this] (const auto& path) {
            CategorySettingValueMap map;
            mLoader.loadSettingsFile(path, map);

            EXPECT_EQ(map, CategorySettingValueMap({
                {CategorySetting("Section", "key"), "value"}
            }));
        });
    }
}
