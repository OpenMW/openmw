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

        TEST(NormalizedTest, shouldSupportConstructorFromNormalizedView)
        {
            const NormalizedView view("foo/bar/baz");
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

        TEST(NormalizedTest, shouldSupportOperatorDivEqual)
        {
            Normalized value("foo/bar");
            value /= NormalizedView("baz");
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(NormalizedTest, shouldSupportOperatorDivEqualWithStringView)
        {
            Normalized value("foo/bar");
            value /= std::string_view("BAZ");
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(NormalizedTest, changeExtensionShouldReplaceAfterLastDot)
        {
            Normalized value("foo/bar.a");
            ASSERT_TRUE(value.changeExtension("so"));
            EXPECT_EQ(value.value(), "foo/bar.so");
        }

        TEST(NormalizedTest, changeExtensionShouldNormalizeExtension)
        {
            Normalized value("foo/bar.a");
            ASSERT_TRUE(value.changeExtension("SO"));
            EXPECT_EQ(value.value(), "foo/bar.so");
        }

        TEST(NormalizedTest, changeExtensionShouldIgnorePathWithoutADot)
        {
            Normalized value("foo/bar");
            ASSERT_FALSE(value.changeExtension("so"));
            EXPECT_EQ(value.value(), "foo/bar");
        }

        TEST(NormalizedTest, changeExtensionShouldIgnorePathWithDotBeforeSeparator)
        {
            Normalized value("foo.bar/baz");
            ASSERT_FALSE(value.changeExtension("so"));
            EXPECT_EQ(value.value(), "foo.bar/baz");
        }

        TEST(NormalizedTest, changeExtensionShouldThrowExceptionOnExtensionWithDot)
        {
            Normalized value("foo.a");
            EXPECT_THROW(value.changeExtension(".so"), std::invalid_argument);
        }

        TEST(NormalizedTest, changeExtensionShouldThrowExceptionOnExtensionWithSeparator)
        {
            Normalized value("foo.a");
            EXPECT_THROW(value.changeExtension("/so"), std::invalid_argument);
        }

        template <class T>
        struct NormalizedOperatorsTest : Test
        {
        };

        TYPED_TEST_SUITE_P(NormalizedOperatorsTest);

        TYPED_TEST_P(NormalizedOperatorsTest, supportsEqual)
        {
            using Type0 = typename TypeParam::Type0;
            using Type1 = typename TypeParam::Type1;
            const Type0 normalized{ "a/foo/bar/baz" };
            const Type1 otherEqual{ "a/foo/bar/baz" };
            const Type1 otherNotEqual{ "b/foo/bar/baz" };
            EXPECT_EQ(normalized, otherEqual);
            EXPECT_EQ(otherEqual, normalized);
            EXPECT_NE(normalized, otherNotEqual);
            EXPECT_NE(otherNotEqual, normalized);
        }

        TYPED_TEST_P(NormalizedOperatorsTest, supportsLess)
        {
            using Type0 = typename TypeParam::Type0;
            using Type1 = typename TypeParam::Type1;
            const Type0 normalized{ "b/foo/bar/baz" };
            const Type1 otherEqual{ "b/foo/bar/baz" };
            const Type1 otherLess{ "a/foo/bar/baz" };
            const Type1 otherGreater{ "c/foo/bar/baz" };
            EXPECT_FALSE(normalized < otherEqual);
            EXPECT_FALSE(otherEqual < normalized);
            EXPECT_LT(otherLess, normalized);
            EXPECT_FALSE(normalized < otherLess);
            EXPECT_LT(normalized, otherGreater);
            EXPECT_FALSE(otherGreater < normalized);
        }

        REGISTER_TYPED_TEST_SUITE_P(NormalizedOperatorsTest, supportsEqual, supportsLess);

        template <class T0, class T1>
        struct TypePair
        {
            using Type0 = T0;
            using Type1 = T1;
        };

        using TypePairs = Types<TypePair<Normalized, Normalized>, TypePair<Normalized, const char*>,
            TypePair<Normalized, std::string>, TypePair<Normalized, std::string_view>,
            TypePair<Normalized, NormalizedView>, TypePair<NormalizedView, Normalized>,
            TypePair<NormalizedView, const char*>, TypePair<NormalizedView, std::string>,
            TypePair<NormalizedView, std::string_view>, TypePair<NormalizedView, NormalizedView>>;

        INSTANTIATE_TYPED_TEST_SUITE_P(Typed, NormalizedOperatorsTest, TypePairs);

        TEST(NormalizedViewTest, shouldSupportConstructorFromNormalized)
        {
            const Normalized value("Foo\\Bar/baz");
            const NormalizedView view(value);
            EXPECT_EQ(view.value(), "foo/bar/baz");
        }

        TEST(NormalizedViewTest, shouldSupportConstexprConstructorFromNormalizedStringLiteral)
        {
            constexpr NormalizedView view("foo/bar/baz");
            EXPECT_EQ(view.value(), "foo/bar/baz");
        }

        TEST(NormalizedViewTest, constructorShouldThrowExceptionOnNotNormalized)
        {
            EXPECT_THROW([] { NormalizedView("Foo\\Bar/baz"); }(), std::invalid_argument);
        }

        TEST(NormalizedView, shouldSupportOperatorDiv)
        {
            const NormalizedView a("foo/bar");
            const NormalizedView b("baz");
            const Normalized result = a / b;
            EXPECT_EQ(result.value(), "foo/bar/baz");
        }
    }
}
