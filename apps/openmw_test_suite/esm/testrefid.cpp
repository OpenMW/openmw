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

        TEST(ESMRefIdTest, indexRefIdHashDiffersForDistinctValues)
        {
            const RefId a = RefId::index(static_cast<RecNameInts>(3), 1);
            const RefId b = RefId::index(static_cast<RecNameInts>(3), 2);
            std::hash<RefId> hash;
            EXPECT_NE(hash(a), hash(b));
        }

        TEST(ESMRefIdTest, indexRefIdHashDiffersForDistinctRecords)
        {
            const RefId a = RefId::index(static_cast<RecNameInts>(1), 3);
            const RefId b = RefId::index(static_cast<RecNameInts>(2), 3);
            std::hash<RefId> hash;
            EXPECT_NE(hash(a), hash(b));
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
            { RefId::formIdRefId(42), "0x2a" },
            { RefId::generated(42), "0x2a" },
            { RefId::index(REC_ARMO, 42), "ARMO:0x2a" },
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
            { RefId::stringRefId("foo"), "\"foo\"" },
            { RefId::stringRefId("BAR"), "\"BAR\"" },
            { RefId::stringRefId(std::string({ 'a', 0, -1, '\n', '\t' })), "\"a\\x0\\xFF\\xA\\x9\"" },
            { RefId::formIdRefId(42), "FormId:0x2a" },
            { RefId::generated(42), "Generated:0x2a" },
            { RefId::index(REC_ARMO, 42), "Index:ARMO:0x2a" },
        };

        INSTANTIATE_TEST_SUITE_P(ESMRefIdToDebugString, ESMRefIdToDebugStringTest, ValuesIn(toDebugStringParams));

        struct ESMRefIdTextTest : TestWithParam<std::pair<RefId, std::string>>
        {
        };

        TEST_P(ESMRefIdTextTest, serializeTextShouldReturnString)
        {
            EXPECT_EQ(GetParam().first.serializeText(), GetParam().second);
        }

        TEST_P(ESMRefIdTextTest, deserializeTextShouldReturnRefId)
        {
            EXPECT_EQ(RefId::deserializeText(GetParam().second), GetParam().first);
        }

        const std::vector<std::pair<RefId, std::string>> serializedRefIds = {
            { RefId(), "" },
            { RefId::stringRefId("foo"), "foo" },
            { RefId::stringRefId("BAR"), "bar" },
            { RefId::stringRefId(std::string({ 'a', 0, -1, '\n', '\t' })), { 'a', 0, -1, '\n', '\t' } },
            { RefId::formIdRefId(0), "FormId:0x0" },
            { RefId::formIdRefId(1), "FormId:0x1" },
            { RefId::formIdRefId(0x1f), "FormId:0x1f" },
            { RefId::formIdRefId(std::numeric_limits<ESM4::FormId>::max()), "FormId:0xffffffff" },
            { RefId::generated(0), "Generated:0x0" },
            { RefId::generated(1), "Generated:0x1" },
            { RefId::generated(0x1f), "Generated:0x1f" },
            { RefId::generated(std::numeric_limits<std::uint64_t>::max()), "Generated:0xffffffffffffffff" },
            { RefId::index(REC_INGR, 0), "Index:INGR:0x0" },
            { RefId::index(REC_INGR, 1), "Index:INGR:0x1" },
            { RefId::index(REC_INGR, 0x1f), "Index:INGR:0x1f" },
            { RefId::index(REC_INGR, std::numeric_limits<std::uint32_t>::max()), "Index:INGR:0xffffffff" },
        };

        INSTANTIATE_TEST_SUITE_P(ESMRefIdText, ESMRefIdTextTest, ValuesIn(serializedRefIds));

        template <class>
        [[maybe_unused]] constexpr bool alwaysFalse = false;

        template <class T>
        struct GenerateRefId
        {
            static_assert(alwaysFalse<T>,
                "There should be specialization for each RefId type. If this assert fails, probably there are no tests "
                "for a new RefId type.");
        };

        template <>
        struct GenerateRefId<EmptyRefId>
        {
            static RefId call() { return RefId(); }
        };

        template <>
        struct GenerateRefId<StringRefId>
        {
            static RefId call() { return RefId::stringRefId("StringRefId"); }
        };

        template <>
        struct GenerateRefId<FormIdRefId>
        {
            static RefId call() { return RefId::formIdRefId(42); }
        };

        template <>
        struct GenerateRefId<GeneratedRefId>
        {
            static RefId call() { return RefId::generated(13); }
        };

        template <>
        struct GenerateRefId<IndexRefId>
        {
            static RefId call() { return RefId::index(REC_BOOK, 7); }
        };

        template <>
        struct GenerateRefId<ESM3ExteriorCellRefId>
        {
            static RefId call() { return RefId::esm3ExteriorCell(-12, 7); }
        };

        template <class T>
        struct ESMRefIdTypesTest : Test
        {
        };

        TYPED_TEST_SUITE_P(ESMRefIdTypesTest);

        TYPED_TEST_P(ESMRefIdTypesTest, serializeThenDeserializeShouldProduceSameValue)
        {
            const RefId refId = GenerateRefId<TypeParam>::call();
            EXPECT_EQ(RefId::deserialize(refId.serialize()), refId);
        }

        TYPED_TEST_P(ESMRefIdTypesTest, serializeTextThenDeserializeTextShouldProduceSameValue)
        {
            const RefId refId = GenerateRefId<TypeParam>::call();
            EXPECT_EQ(RefId::deserializeText(refId.serializeText()), refId);
        }

        TYPED_TEST_P(ESMRefIdTypesTest, shouldBeEqualToItself)
        {
            const RefId a = GenerateRefId<TypeParam>::call();
            const RefId b = GenerateRefId<TypeParam>::call();
            EXPECT_EQ(a, b);
        }

        TYPED_TEST_P(ESMRefIdTypesTest, shouldNotBeNotEqualToItself)
        {
            const RefId a = GenerateRefId<TypeParam>::call();
            const RefId b = GenerateRefId<TypeParam>::call();
            EXPECT_FALSE(a != b) << a;
        }

        TYPED_TEST_P(ESMRefIdTypesTest, shouldBeNotLessThanItself)
        {
            const RefId a = GenerateRefId<TypeParam>::call();
            const RefId b = GenerateRefId<TypeParam>::call();
            EXPECT_FALSE(a < b) << a;
        }

        REGISTER_TYPED_TEST_SUITE_P(ESMRefIdTypesTest, serializeThenDeserializeShouldProduceSameValue,
            serializeTextThenDeserializeTextShouldProduceSameValue, shouldBeEqualToItself, shouldNotBeNotEqualToItself,
            shouldBeNotLessThanItself);

        template <class>
        struct RefIdTypes;

        template <class... Args>
        struct RefIdTypes<std::variant<Args...>>
        {
            using Type = Types<Args...>;
        };

        using RefIdTypeParams = typename RefIdTypes<RefId::Value>::Type;

        INSTANTIATE_TYPED_TEST_SUITE_P(RefIdTypes, ESMRefIdTypesTest, RefIdTypeParams);
    }
}
