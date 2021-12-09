#include <gtest/gtest.h>
#include "components/misc/stringops.hpp"
#include "components/misc/algorithm.hpp"

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
        mDataVec = std::vector<std::string>(data, data+sizeof(data)/sizeof(data[0]));
        std::sort(mDataVec.begin(), mDataVec.end(), Misc::StringUtils::ciLess);
    }

    bool matches(const std::string& keyword)
    {
        return Misc::partialBinarySearch(mDataVec.begin(), mDataVec.end(), keyword) != mDataVec.end();
    }
};

TEST_F(PartialBinarySearchTest, partial_binary_search_test)
{
    EXPECT_TRUE( matches("Head 01") );
    EXPECT_TRUE( matches("Head") );
    EXPECT_TRUE( matches("Tri Head 01") );
    EXPECT_TRUE( matches("Tri Head") );
    EXPECT_TRUE( matches("tri head") );
    EXPECT_TRUE( matches("Tri bip01") );
    EXPECT_TRUE( matches("bip01") );
    EXPECT_TRUE( matches("bip01 head") );
    EXPECT_TRUE( matches("Bip01 L Hand") );
    EXPECT_TRUE( matches("BIp01 r Clavicle") );
    EXPECT_TRUE( matches("Bip01 SpiNe1") );

    EXPECT_FALSE( matches("") );
    EXPECT_FALSE( matches("Node Bip01") );
    EXPECT_FALSE( matches("Hea") );
    EXPECT_FALSE( matches(" Head") );
    EXPECT_FALSE( matches("Tri  Head") );
}

TEST_F (PartialBinarySearchTest, ci_test)
{
    EXPECT_TRUE (Misc::StringUtils::lowerCase("ASD") == "asd");

    // test to make sure system locale is not used
    std::string unicode1 = "\u04151 \u0418"; // CYRILLIC CAPITAL LETTER IE, CYRILLIC CAPITAL LETTER I
    EXPECT_TRUE( Misc::StringUtils::lowerCase(unicode1) == unicode1 );
}

namespace
{
    using ::Misc::StringUtils;
    using namespace ::testing;

    template <class T>
    struct MiscStringUtilsCiEqualEmptyTest : Test {};

    TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualEmptyTest);

    TYPED_TEST_P(MiscStringUtilsCiEqualEmptyTest, empty_strings_should_be_equal)
    {
        EXPECT_TRUE(StringUtils::ciEqual(typename TypeParam::first_type {}, typename TypeParam::second_type {}));
    }

    REGISTER_TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualEmptyTest,
        empty_strings_should_be_equal
    );

    using EmptyStringTypePairsTypes = Types<
        std::pair<std::string, std::string>,
        std::pair<std::string, std::string_view>,
        std::pair<std::string, const char[1]>,
        std::pair<std::string_view, std::string>,
        std::pair<std::string_view, std::string_view>,
        std::pair<std::string_view, const char[1]>,
        std::pair<const char[1], std::string>,
        std::pair<const char[1], std::string_view>,
        std::pair<const char[1], const char[1]>
    >;

    INSTANTIATE_TYPED_TEST_SUITE_P(EmptyStringTypePairs, MiscStringUtilsCiEqualEmptyTest, EmptyStringTypePairsTypes);

    template <class T>
    struct MiscStringUtilsCiEqualNotEmptyTest : Test {};

    TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualNotEmptyTest);

    using RawValue = const char[4];

    constexpr RawValue foo = "foo";
    constexpr RawValue fooUpper = "FOO";
    constexpr RawValue bar = "bar";

    template <typename T>
    using Value = std::conditional_t<std::is_same_v<T, RawValue>, RawValue&, T>;

    TYPED_TEST_P(MiscStringUtilsCiEqualNotEmptyTest, same_strings_should_be_equal)
    {
        const Value<typename TypeParam::first_type> a {foo};
        const Value<typename TypeParam::second_type> b {foo};
        EXPECT_TRUE(StringUtils::ciEqual(a, b)) << a << "\n" << b;
    }

    TYPED_TEST_P(MiscStringUtilsCiEqualNotEmptyTest, same_strings_with_different_case_sensetivity_should_be_equal)
    {
        const Value<typename TypeParam::first_type> a {foo};
        const Value<typename TypeParam::second_type> b {fooUpper};
        EXPECT_TRUE(StringUtils::ciEqual(a, b)) << a << "\n" << b;
    }

    TYPED_TEST_P(MiscStringUtilsCiEqualNotEmptyTest, different_strings_content_should_not_be_equal)
    {
        const Value<typename TypeParam::first_type> a {foo};
        const Value<typename TypeParam::second_type> b {bar};
        EXPECT_FALSE(StringUtils::ciEqual(a, b)) << a << "\n" << b;
    }

    REGISTER_TYPED_TEST_SUITE_P(MiscStringUtilsCiEqualNotEmptyTest,
        same_strings_should_be_equal,
        same_strings_with_different_case_sensetivity_should_be_equal,
        different_strings_content_should_not_be_equal
    );

    using NotEmptyStringTypePairsTypes = Types<
        std::pair<std::string, std::string>,
        std::pair<std::string, std::string_view>,
        std::pair<std::string, const char[4]>,
        std::pair<std::string_view, std::string>,
        std::pair<std::string_view, std::string_view>,
        std::pair<std::string_view, const char[4]>,
        std::pair<const char[4], std::string>,
        std::pair<const char[4], std::string_view>,
        std::pair<const char[4], const char[4]>
    >;

    INSTANTIATE_TYPED_TEST_SUITE_P(NotEmptyStringTypePairs, MiscStringUtilsCiEqualNotEmptyTest, NotEmptyStringTypePairsTypes);

    TEST(MiscStringUtilsCiEqualTest, string_with_different_length_should_not_be_equal)
    {
        EXPECT_FALSE(StringUtils::ciEqual(std::string("a"), std::string("aa")));
    }
}
