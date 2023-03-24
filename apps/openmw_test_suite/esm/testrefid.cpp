#include <components/esm/refid.hpp>

#include <gtest/gtest.h>

#include <functional>
#include <map>
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

        TEST(ESMRefIdTest, equalityIsDefinedForStringRefIdAndRefId)
        {
            const StringRefId stringRefId("ref_id");
            const RefId refId = RefId::stringRefId("REF_ID");
            EXPECT_EQ(stringRefId, refId);
        }

        TEST(ESMRefIdTest, equalityIsDefinedForFormRefIdAndRefId)
        {
            const FormIdRefId formIdRefId(42);
            const RefId refId = RefId::formIdRefId(42);
            EXPECT_EQ(formIdRefId, refId);
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

        TEST(ESMRefIdTest, lessThanIsDefinedForStringRefIdAndRefId)
        {
            const StringRefId stringRefId("a");
            const RefId refId = RefId::stringRefId("B");
            EXPECT_LT(stringRefId, refId);
        }

        TEST(ESMRefIdTest, lessThanIsDefinedForFormRefIdAndRefId)
        {
            const FormIdRefId formIdRefId(13);
            const RefId refId = RefId::formIdRefId(42);
            EXPECT_LT(formIdRefId, refId);
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

        TEST(ESMRefIdTest, hasCaseInsensitiveStrongOrderWithStringView)
        {
            const RefId a = RefId::stringRefId("a");
            const std::string_view b = "B";
            const RefId c = RefId::stringRefId("c");
            EXPECT_LT(a, b);
            EXPECT_LT(b, c);
        }

        TEST(ESMRefIdTest, stringRefIdHasStrongOrderWithFormId)
        {
            const RefId stringRefId = RefId::stringRefId("a");
            const RefId formIdRefId = RefId::formIdRefId(42);
            EXPECT_TRUE(stringRefId < formIdRefId);
            EXPECT_FALSE(formIdRefId < stringRefId);
        }

        TEST(ESMRefIdTest, formIdRefIdHasStrongOrderWithStringView)
        {
            const RefId formIdRefId = RefId::formIdRefId(42);
            const std::string_view stringView = "42";
            EXPECT_TRUE(stringView < formIdRefId);
            EXPECT_FALSE(formIdRefId < stringView);
        }

        TEST(ESMRefIdTest, canBeUsedAsMapKeyWithLookupByStringView)
        {
            const std::map<RefId, int, std::less<>> map({ { RefId::stringRefId("a"), 42 } });
            EXPECT_EQ(map.count("A"), 1);
        }

        TEST(ESMRefIdTest, canBeUsedAsLookupKeyForMapWithStringKey)
        {
            const std::map<std::string, int, std::less<>> map({ { "a", 42 } });
            EXPECT_EQ(map.count(RefId::stringRefId("A")), 1);
        }

        TEST(ESMRefIdTest, stringRefIdIsNotEqualToFormId)
        {
            const RefId stringRefId = RefId::stringRefId("\0");
            const RefId formIdRefId = RefId::formIdRefId(0);
            EXPECT_NE(stringRefId, formIdRefId);
        }

        struct ESMRefIdToStringTest : TestWithParam<std::pair<RefId, std::string>>
        {
        };

        TEST_P(ESMRefIdToStringTest, toString)
        {
            const RefId& refId = GetParam().first;
            const std::string& string = GetParam().second;
            EXPECT_EQ(refId.toString(), string);
        }

        const std::vector<std::pair<RefId, std::string>> toStringParams = {
            { RefId(), std::string() },
            { RefId::stringRefId("foo"), "foo" },
            { RefId::stringRefId(std::string({ 'a', 0, -1, '\n', '\t' })), { 'a', 0, -1, '\n', '\t' } },
            { RefId::formIdRefId(42), "42" },
            { RefId::generated(42), "42" },
            { RefId::index(REC_ARMO, 42), "ARMO, 42" },
        };

        INSTANTIATE_TEST_SUITE_P(ESMRefIdToString, ESMRefIdToStringTest, ValuesIn(toStringParams));

        struct ESMRefIdToDebugStringTest : TestWithParam<std::pair<RefId, std::string>>
        {
        };

        TEST_P(ESMRefIdToDebugStringTest, toDebugString)
        {
            const RefId& refId = GetParam().first;
            const std::string& debugString = GetParam().second;
            EXPECT_EQ(refId.toDebugString(), debugString);
        }

        TEST_P(ESMRefIdToDebugStringTest, toStream)
        {
            const RefId& refId = GetParam().first;
            const std::string& debugString = GetParam().second;
            std::ostringstream stream;
            stream << refId;
            EXPECT_EQ(stream.str(), debugString);
        }

        const std::vector<std::pair<RefId, std::string>> toDebugStringParams = {
            { RefId(), "Empty{}" },
            { RefId::stringRefId("foo"), "String{foo}" },
            { RefId::stringRefId(std::string({ 'a', 0, -1, '\n', '\t' })), "String{a\\x0\\xFF\\xA\\x9}" },
            { RefId::formIdRefId(42), "FormId{42}" },
            { RefId::generated(42), "Generated{42}" },
            { RefId::index(REC_ARMO, 42), "Index{ARMO, 42}" },
        };

        INSTANTIATE_TEST_SUITE_P(ESMRefIdToDebugString, ESMRefIdToDebugStringTest, ValuesIn(toDebugStringParams));

        template <class T>
        RefId generateRefId();

        template <>
        RefId generateRefId<EmptyRefId>()
        {
            return RefId();
        }

        template <>
        RefId generateRefId<StringRefId>()
        {
            return RefId::stringRefId("StringRefId");
        }

        template <>
        RefId generateRefId<FormIdRefId>()
        {
            return RefId::formIdRefId(42);
        }

        template <>
        RefId generateRefId<GeneratedRefId>()
        {
            return RefId::generated(13);
        }

        template <>
        RefId generateRefId<IndexRefId>()
        {
            return RefId::index(REC_BOOK, 7);
        }

        template <class T>
        struct ESMRefIdSerializeDeserializeTest : Test
        {
        };

        TYPED_TEST_SUITE_P(ESMRefIdSerializeDeserializeTest);

        TYPED_TEST_P(ESMRefIdSerializeDeserializeTest, serializeThenDeserializeShouldProduceSameValue)
        {
            const RefId refId = generateRefId<TypeParam>();
            EXPECT_EQ(RefId::deserialize(refId.serialize()), refId);
        }

        REGISTER_TYPED_TEST_SUITE_P(ESMRefIdSerializeDeserializeTest, serializeThenDeserializeShouldProduceSameValue);

        using RefIdTypeParams = Types<EmptyRefId, StringRefId, FormIdRefId, GeneratedRefId, IndexRefId>;

        INSTANTIATE_TYPED_TEST_SUITE_P(RefIdTypes, ESMRefIdSerializeDeserializeTest, RefIdTypeParams);
    }
}
