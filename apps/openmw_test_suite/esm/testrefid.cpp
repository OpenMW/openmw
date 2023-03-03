#include <components/esm/refid.hpp>

#include <gtest/gtest.h>

#include <functional>
#include <string>

namespace ESM
{
    namespace
    {
        using namespace ::testing;

        TEST(ESMRefIdTest, defaultConstructedIsEmpty)
        {
            const RefId refId;
            EXPECT_TRUE(refId.empty());
        }

        TEST(ESMRefIdTest, stringRefIdIsNotEmpty)
        {
            const RefId refId = RefId::stringRefId("ref_id");
            EXPECT_FALSE(refId.empty());
        }

        TEST(ESMRefIdTest, formIdRefIdIsNotEmpty)
        {
            const RefId refId = RefId::formIdRefId(42);
            EXPECT_FALSE(refId.empty());
        }

        TEST(ESMRefIdTest, defaultConstructedIsEqualToItself)
        {
            const RefId refId;
            EXPECT_EQ(refId, refId);
        }

        TEST(ESMRefIdTest, defaultConstructedIsEqualToDefaultConstructed)
        {
            const RefId a;
            const RefId b;
            EXPECT_EQ(a, b);
        }

        TEST(ESMRefIdTest, defaultConstructedIsNotEqualToDebugStringRefId)
        {
            const RefId a;
            const RefId b = RefId::stringRefId("b");
            EXPECT_NE(a, b);
        }

        TEST(ESMRefIdTest, defaultConstructedIsNotEqualToFormIdRefId)
        {
            const RefId a;
            const RefId b = RefId::formIdRefId(42);
            EXPECT_NE(a, b);
        }

        TEST(ESMRefIdTest, defaultConstructedIsNotEqualToDebugStringLiteral)
        {
            const RefId a;
            EXPECT_NE(a, "foo");
        }

        TEST(ESMRefIdTest, stringRefIdIsEqualToTheSameStringLiteralValue)
        {
            const RefId refId = RefId::stringRefId("ref_id");
            EXPECT_EQ(refId, "ref_id");
        }

        TEST(ESMRefIdTest, stringRefIdIsCaseInsensitiveEqualToTheSameStringLiteralValue)
        {
            const RefId refId = RefId::stringRefId("ref_id");
            EXPECT_EQ(refId, "REF_ID");
        }

        TEST(ESMRefIdTest, stringRefIdIsEqualToTheSameStringRefId)
        {
            const RefId a = RefId::stringRefId("ref_id");
            const RefId b = RefId::stringRefId("ref_id");
            EXPECT_EQ(a, b);
        }

        TEST(ESMRefIdTest, stringRefIdIsCaseInsensitiveEqualToTheSameStringRefId)
        {
            const RefId lower = RefId::stringRefId("ref_id");
            const RefId upper = RefId::stringRefId("REF_ID");
            EXPECT_EQ(lower, upper);
        }

        TEST(ESMRefIdTest, stringRefIdIsEqualToItself)
        {
            const RefId refId = RefId::stringRefId("ref_id");
            EXPECT_EQ(refId, refId);
        }

        TEST(ESMRefIdTest, stringRefIdIsCaseInsensitiveLessByContent)
        {
            const RefId a = RefId::stringRefId("a");
            const RefId b = RefId::stringRefId("B");
            EXPECT_LT(a, b);
        }

        TEST(ESMRefIdTest, stringRefIdHasCaseInsensitiveHash)
        {
            const RefId lower = RefId::stringRefId("a");
            const RefId upper = RefId::stringRefId("A");
            const std::hash<RefId> hash;
            EXPECT_EQ(hash(lower), hash(upper));
        }

        TEST(ESMRefIdTest, hasCaseInsensitiveEqualityWithStringView)
        {
            const RefId a = RefId::stringRefId("a");
            const std::string_view b = "A";
            EXPECT_EQ(a, b);
        }

        TEST(ESMRefIdTest, hasCaseInsensitiveLessWithStringView)
        {
            const RefId a = RefId::stringRefId("a");
            const std::string_view b = "B";
            EXPECT_LT(a, b);
        }
    }
}
