#include <components/vfs/pathutil.hpp>

#include <gtest/gtest.h>

#include <sstream>

namespace VFS::Path
{
    namespace
    {
        using namespace testing;

        TEST(NormalizedTest, shouldSupportDefaultConstructor)
        {
            const Normalized value;
            EXPECT_EQ(value.value(), "");
        }

        TEST(NormalizedTest, shouldSupportConstructorFromString)
        {
            const std::string string("Foo\\Bar/baz");
            const Normalized value(string);
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(NormalizedTest, shouldSupportConstructorFromConstCharPtr)
        {
            const char* const ptr = "Foo\\Bar/baz";
            const Normalized value(ptr);
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(NormalizedTest, shouldSupportConstructorFromStringView)
        {
            const std::string_view view = "Foo\\Bar/baz";
            const Normalized value(view);
            EXPECT_EQ(value.view(), "foo/bar/baz");
        }

        TEST(NormalizedTest, supportMovingValueOut)
        {
            Normalized value("Foo\\Bar/baz");
            EXPECT_EQ(std::move(value).value(), "foo/bar/baz");
            EXPECT_EQ(value.value(), "");
        }

        TEST(NormalizedTest, isNotEqualToNotNormalized)
        {
            const Normalized value("Foo\\Bar/baz");
            EXPECT_NE(value.value(), "Foo\\Bar/baz");
        }

        TEST(NormalizedTest, shouldSupportOperatorLeftShiftToOStream)
        {
            const Normalized value("Foo\\Bar/baz");
            std::stringstream stream;
            stream << value;
            EXPECT_EQ(stream.str(), "foo/bar/baz");
        }

        template <class T>
        struct NormalizedOperatorsTest : Test
        {
        };

        TYPED_TEST_SUITE_P(NormalizedOperatorsTest);

        TYPED_TEST_P(NormalizedOperatorsTest, supportsEqual)
        {
            const Normalized normalized("a/foo/bar/baz");
            const TypeParam otherEqual{ "a/foo/bar/baz" };
            const TypeParam otherNotEqual{ "b/foo/bar/baz" };
            EXPECT_EQ(normalized, otherEqual);
            EXPECT_EQ(otherEqual, normalized);
            EXPECT_NE(normalized, otherNotEqual);
            EXPECT_NE(otherNotEqual, normalized);
        }

        TYPED_TEST_P(NormalizedOperatorsTest, supportsLess)
        {
            const Normalized normalized("b/foo/bar/baz");
            const TypeParam otherEqual{ "b/foo/bar/baz" };
            const TypeParam otherLess{ "a/foo/bar/baz" };
            const TypeParam otherGreater{ "c/foo/bar/baz" };
            EXPECT_FALSE(normalized < otherEqual);
            EXPECT_FALSE(otherEqual < normalized);
            EXPECT_LT(otherLess, normalized);
            EXPECT_FALSE(normalized < otherLess);
            EXPECT_LT(normalized, otherGreater);
            EXPECT_FALSE(otherGreater < normalized);
        }

        REGISTER_TYPED_TEST_SUITE_P(NormalizedOperatorsTest, supportsEqual, supportsLess);

        using StringTypes = Types<Normalized, const char*, std::string, std::string_view>;

        INSTANTIATE_TYPED_TEST_SUITE_P(Typed, NormalizedOperatorsTest, StringTypes);
    }
}
