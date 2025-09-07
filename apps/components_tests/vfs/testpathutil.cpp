#include <components/vfs/pathutil.hpp>

#include <gtest/gtest.h>

#include <sstream>

namespace VFS::Path
{
    namespace
    {
        using namespace testing;

        TEST(VFSPathNormalizedTest, shouldSupportDefaultConstructor)
        {
            const Normalized value;
            EXPECT_EQ(value.value(), "");
        }

        TEST(VFSPathNormalizedTest, shouldSupportConstructorFromString)
        {
            const std::string string("Foo\\Bar/baz");
            const Normalized value(string);
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedTest, shouldSupportConstructorFromConstCharPtr)
        {
            const char* const ptr = "Foo\\Bar/baz";
            const Normalized value(ptr);
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedTest, shouldSupportConstructorFromStringView)
        {
            const std::string_view view = "Foo\\Bar/baz";
            const Normalized value(view);
            EXPECT_EQ(value.view(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedTest, shouldSupportConstructorFromNormalizedView)
        {
            const NormalizedView view("foo/bar/baz");
            const Normalized value(view);
            EXPECT_EQ(value.view(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedTest, supportMovingValueOut)
        {
            Normalized value("Foo\\Bar/baz");
            EXPECT_EQ(std::move(value).value(), "foo/bar/baz");
            EXPECT_EQ(value.value(), "");
        }

        TEST(VFSPathNormalizedTest, isNotEqualToNotNormalized)
        {
            const Normalized value("Foo\\Bar/baz");
            EXPECT_NE(value.value(), "Foo\\Bar/baz");
        }

        TEST(VFSPathNormalizedTest, shouldSupportOperatorLeftShiftToOStream)
        {
            const Normalized value("Foo\\Bar/baz");
            std::stringstream stream;
            stream << value;
            EXPECT_EQ(stream.str(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedTest, shouldSupportOperatorDivEqual)
        {
            Normalized value("foo/bar");
            value /= NormalizedView("baz");
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedTest, shouldSupportOperatorDivEqualWithStringView)
        {
            Normalized value("foo/bar");
            value /= std::string_view("BAZ");
            EXPECT_EQ(value.value(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedTest, changeExtensionShouldReplaceAfterLastDot)
        {
            Normalized value("foo/bar.a");
            ASSERT_TRUE(value.changeExtension("so"));
            EXPECT_EQ(value.value(), "foo/bar.so");
        }

        TEST(VFSPathNormalizedTest, changeExtensionShouldNormalizeExtension)
        {
            Normalized value("foo/bar.a");
            ASSERT_TRUE(value.changeExtension("SO"));
            EXPECT_EQ(value.value(), "foo/bar.so");
        }

        TEST(VFSPathNormalizedTest, changeExtensionShouldIgnorePathWithoutADot)
        {
            Normalized value("foo/bar");
            ASSERT_FALSE(value.changeExtension("so"));
            EXPECT_EQ(value.value(), "foo/bar");
        }

        TEST(VFSPathNormalizedTest, changeExtensionShouldIgnorePathWithDotBeforeSeparator)
        {
            Normalized value("foo.bar/baz");
            ASSERT_FALSE(value.changeExtension("so"));
            EXPECT_EQ(value.value(), "foo.bar/baz");
        }

        TEST(VFSPathNormalizedTest, changeExtensionShouldThrowExceptionOnExtensionWithDot)
        {
            Normalized value("foo.a");
            EXPECT_THROW(value.changeExtension(".so"), std::invalid_argument);
        }

        TEST(VFSPathNormalizedTest, changeExtensionShouldThrowExceptionOnExtensionWithSeparator)
        {
            Normalized value("foo.a");
            EXPECT_THROW(value.changeExtension("/so"), std::invalid_argument);
        }

        template <class T>
        struct VFSPathNormalizedOperatorsTest : Test
        {
        };

        TYPED_TEST_SUITE_P(VFSPathNormalizedOperatorsTest);

        TYPED_TEST_P(VFSPathNormalizedOperatorsTest, supportsEqual)
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

        TYPED_TEST_P(VFSPathNormalizedOperatorsTest, supportsLess)
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

        REGISTER_TYPED_TEST_SUITE_P(VFSPathNormalizedOperatorsTest, supportsEqual, supportsLess);

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

        INSTANTIATE_TYPED_TEST_SUITE_P(Typed, VFSPathNormalizedOperatorsTest, TypePairs);

        TEST(VFSPathNormalizedViewTest, shouldSupportConstructorFromNormalized)
        {
            const Normalized value("Foo\\Bar/baz");
            const NormalizedView view(value);
            EXPECT_EQ(view.value(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedViewTest, shouldSupportConstexprConstructorFromNormalizedStringLiteral)
        {
            constexpr NormalizedView view("foo/bar/baz");
            EXPECT_EQ(view.value(), "foo/bar/baz");
        }

        TEST(VFSPathNormalizedViewTest, constructorShouldThrowExceptionOnNotNormalized)
        {
            EXPECT_THROW([] { NormalizedView("Foo\\Bar/baz"); }(), std::invalid_argument);
        }

        TEST(VFSPathNormalizedViewTest, shouldSupportOperatorDiv)
        {
            const NormalizedView a("foo/bar");
            const NormalizedView b("baz");
            const Normalized result = a / b;
            EXPECT_EQ(result.value(), "foo/bar/baz");
        }
    }
}
