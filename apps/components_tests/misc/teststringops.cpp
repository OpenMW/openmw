#include <gtest/gtest.h>

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/format.hpp>
#include <components/misc/strings/lower.hpp>

#include <components/misc/algorithm.hpp>

#include <string>
#include <string_view>
#include <type_traits>

struct PartialBinarySearchTest : public ::testing::Test
{
protected:
    std::vector<std::string> mDataVec;
    void SetUp() override
    {
        const char* data[] = { "Head", "Chest", "Tri Head", "Tri Chest", "Bip01", "Tri Bip01" };
        mDataVec = std::vector<std::string>(data, data + sizeof(data) / sizeof(data[0]));
        std::sort(mDataVec.begin(), mDataVec.end(), Misc::StringUtils::ciLess);
    }

    bool matches(const std::string& keyword)
    {
        return Misc::partialBinarySearch(mDataVec.begin(), mDataVec.end(), keyword) != mDataVec.end();
    }
};

TEST_F(PartialBinarySearchTest, partial_binary_search_test)
{
    EXPECT_TRUE(matches("Head 01"));
    EXPECT_TRUE(matches("Head"));
    EXPECT_TRUE(matches("Tri Head 01"));
    EXPECT_TRUE(matches("Tri Head"));
    EXPECT_TRUE(matches("tri head"));
    EXPECT_TRUE(matches("Tri bip01"));
    EXPECT_TRUE(matches("bip01"));
    EXPECT_TRUE(matches("bip01 head"));
    EXPECT_TRUE(matches("Bip01 L Hand"));
    EXPECT_TRUE(matches("BIp01 r Clavicle"));
    EXPECT_TRUE(matches("Bip01 SpiNe1"));

    EXPECT_FALSE(matches(""));
    EXPECT_FALSE(matches("Node Bip01"));
    EXPECT_FALSE(matches("Hea"));
    EXPECT_FALSE(matches(" Head"));
    EXPECT_FALSE(matches("Tri  Head"));
}

TEST_F(PartialBinarySearchTest, ci_test)
{
    EXPECT_TRUE(Misc::StringUtils::lowerCase("ASD") == "asd");

    // test to make sure system locale is not used
    std::string unicode1 = "\u04151 \u0418"; // CYRILLIC CAPITAL LETTER IE, CYRILLIC CAPITAL LETTER I
    EXPECT_TRUE(Misc::StringUtils::lowerCase(unicode1) == unicode1);
}

namespace
{
    using namespace ::Misc::StringUtils;
    using namespace ::testing;

    template <class T>
    struct MiscStringUtilsCiEqualEmptyTest : Test
    {
    };

    TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualEmptyTest);

    TYPED_TEST_P(MiscStringUtilsCiEqualEmptyTest, empty_strings_should_be_equal)
    {
        const typename TypeParam::first_type a{};
        const typename TypeParam::second_type b{};
        EXPECT_TRUE(ciEqual(a, b));
    }

    REGISTER_TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualEmptyTest, empty_strings_should_be_equal);

    using EmptyStringTypePairsTypes = Types<std::pair<std::string, std::string>,
        std::pair<std::string, std::string_view>, std::pair<std::string, const char[1]>,
        std::pair<std::string_view, std::string>, std::pair<std::string_view, std::string_view>,
        std::pair<std::string_view, const char[1]>, std::pair<const char[1], std::string>,
        std::pair<const char[1], std::string_view>, std::pair<const char[1], const char[1]>>;

    INSTANTIATE_TYPED_TEST_SUITE_P(EmptyStringTypePairs, MiscStringUtilsCiEqualEmptyTest, EmptyStringTypePairsTypes);

    template <class T>
    struct MiscStringUtilsCiEqualNotEmptyTest : Test
    {
    };

    TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualNotEmptyTest);

    using RawValue = const char[4];

    constexpr RawValue foo = "f0#";
    constexpr RawValue fooUpper = "F0#";
    constexpr RawValue bar = "bar";

    template <typename T>
    using Value = std::conditional_t<std::is_same_v<T, RawValue>, RawValue&, T>;

    TYPED_TEST_P(MiscStringUtilsCiEqualNotEmptyTest, same_strings_should_be_equal)
    {
        const Value<typename TypeParam::first_type> a{ foo };
        const Value<typename TypeParam::second_type> b{ foo };
        EXPECT_TRUE(ciEqual(a, b)) << a << "\n" << b;
    }

    TYPED_TEST_P(MiscStringUtilsCiEqualNotEmptyTest, same_strings_with_different_case_sensetivity_should_be_equal)
    {
        const Value<typename TypeParam::first_type> a{ foo };
        const Value<typename TypeParam::second_type> b{ fooUpper };
        EXPECT_TRUE(ciEqual(a, b)) << a << "\n" << b;
    }

    TYPED_TEST_P(MiscStringUtilsCiEqualNotEmptyTest, different_strings_content_should_not_be_equal)
    {
        const Value<typename TypeParam::first_type> a{ foo };
        const Value<typename TypeParam::second_type> b{ bar };
        EXPECT_FALSE(ciEqual(a, b)) << a << "\n" << b;
    }

    REGISTER_TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualNotEmptyTest, same_strings_should_be_equal,
        same_strings_with_different_case_sensetivity_should_be_equal, different_strings_content_should_not_be_equal);

    using NotEmptyStringTypePairsTypes = Types<std::pair<std::string, std::string>,
        std::pair<std::string, std::string_view>, std::pair<std::string, const char[4]>,
        std::pair<std::string_view, std::string>, std::pair<std::string_view, std::string_view>,
        std::pair<std::string_view, const char[4]>, std::pair<const char[4], std::string>,
        std::pair<const char[4], std::string_view>, std::pair<const char[4], const char[4]>>;

    INSTANTIATE_TYPED_TEST_SUITE_P(
        NotEmptyStringTypePairs, MiscStringUtilsCiEqualNotEmptyTest, NotEmptyStringTypePairsTypes);

    TEST(MiscStringUtilsCiEqualTest, string_with_different_length_should_not_be_equal)
    {
        EXPECT_FALSE(ciEqual(std::string("a"), std::string("aa")));
    }

    TEST(MiscStringsCiStartsWith, empty_string_should_start_with_empty_prefix)
    {
        EXPECT_TRUE(ciStartsWith(std::string_view(), std::string_view()));
    }

    TEST(MiscStringsCiStartsWith, string_should_start_with_empty_prefix)
    {
        EXPECT_TRUE(ciStartsWith("foo", std::string_view()));
    }

    TEST(MiscStringsCiStartsWith, string_should_start_with_own_prefix)
    {
        std::string string = "some string";
        EXPECT_TRUE(ciStartsWith(string, string.substr(0, 4)));
    }

    TEST(MiscStringsCiStartsWith, string_should_start_with_the_same_value)
    {
        EXPECT_TRUE(ciStartsWith("foo", "foo"));
    }

    TEST(MiscStringsCiStartsWith, string_should_not_start_with_not_its_prefix)
    {
        EXPECT_FALSE(ciStartsWith("some string", "foo"));
    }

    TEST(MiscStringsCiStartsWith, string_should_not_start_with_longer_string_having_matching_prefix)
    {
        EXPECT_FALSE(ciStartsWith("foo", "foo bar"));
    }

    TEST(MiscStringsCiStartsWith, should_be_case_insensitive)
    {
        EXPECT_TRUE(ciStartsWith("foo bar", "FOO"));
    }

    TEST(MiscStringsFormat, string_format)
    {
        std::string f = "1%s2";
        EXPECT_EQ(Misc::StringUtils::format(f, ""), "12");
    }

    TEST(MiscStringsFormat, string_format_arg)
    {
        std::string arg = "12";
        EXPECT_EQ(Misc::StringUtils::format("1%s2", arg), "1122");
    }

    TEST(MiscStringsFormat, string_view_format_arg)
    {
        std::string f = "1%s2";
        std::string_view view = "12";
        EXPECT_EQ(Misc::StringUtils::format(f, view), "1122");
        EXPECT_EQ(Misc::StringUtils::format(f, view.substr(0, 1)), "112");
        EXPECT_EQ(Misc::StringUtils::format(f, view.substr(1, 1)), "122");
        EXPECT_EQ(Misc::StringUtils::format(f, view.substr(2)), "12");
    }

    TEST(MiscStringsCiFind, should_return_zero_for_2_empty_strings)
    {
        EXPECT_EQ(ciFind(std::string_view(), std::string_view()), 0);
    }

    TEST(MiscStringsCiFind, should_return_zero_when_looking_for_empty_string)
    {
        EXPECT_EQ(ciFind("foo", std::string_view()), 0);
    }

    TEST(MiscStringsCiFind, should_return_npos_for_longer_substring)
    {
        EXPECT_EQ(ciFind("a", "aa"), std::string_view::npos);
    }

    TEST(MiscStringsCiFind, should_return_zero_for_the_same_string)
    {
        EXPECT_EQ(ciFind("foo", "foo"), 0);
    }

    TEST(MiscStringsCiFind, should_return_first_position_of_substring)
    {
        EXPECT_EQ(ciFind("foobar foobar", "bar"), 3);
    }

    TEST(MiscStringsCiFind, should_be_case_insensitive)
    {
        EXPECT_EQ(ciFind("foobar", "BAR"), 3);
    }

    TEST(MiscStringsCiFind, should_return_npos_for_absent_substring)
    {
        EXPECT_EQ(ciFind("foobar", "baz"), std::string_view::npos);
    }
}
