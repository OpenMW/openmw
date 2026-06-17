#include <components/esm/refid.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/testing/expecterror.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>
#include <map>
#include <string>

MATCHER(IsPrint, "")
{
    return std::isprint(arg) != 0;
}

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
            const RefId refId = RefId::formIdRefId({ .mIndex = 42, .mContentFile = 0 });
            EXPECT_FALSE(refId.empty());
        }

        TEST(ESMRefIdTest, FormIdRefIdMustHaveContentFile)
        {
            EXPECT_TRUE(RefId(FormId()).empty());
            EXPECT_ERROR(RefId(FormId{ .mIndex = 1, .mContentFile = -1 }), "RefId can't be a generated FormId");
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
            const RefId b = RefId::formIdRefId({ .mIndex = 42, .mContentFile = 0 });
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

        TEST(ESMRefIdTest, equalityIsDefinedForFormIdAndRefId)
        {
            const FormId formId{ .mIndex = 42, .mContentFile = 0 };
            EXPECT_EQ(formId, RefId(formId));
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

        TEST(ESMRefIdTest, stringRefIdDeserializationReturnsEmptyRefIdForNonExistentValues)
        {
            RefId id = RefId::deserializeText("this stringrefid should not exist");
            EXPECT_TRUE(id.empty());
        }

        TEST(ESMRefIdTest, lessThanIsDefinedForStringRefIdAndRefId)
        {
            const StringRefId stringRefId("a");
            const RefId refId = RefId::stringRefId("B");
            EXPECT_LT(stringRefId, refId);
        }

        TEST(ESMRefIdTest, lessThanIsDefinedForFormRefIdAndRefId)
        {
            const FormId formId{ .mIndex = 13, .mContentFile = 0 };
            const RefId refId = RefId(FormId{ .mIndex = 42, .mContentFile = 0 });
            EXPECT_LT(formId, refId);
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
            const RefId formIdRefId = RefId::formIdRefId({ .mIndex = 42, .mContentFile = 0 });
            EXPECT_TRUE(stringRefId < formIdRefId);
            EXPECT_FALSE(formIdRefId < stringRefId);
        }

        TEST(ESMRefIdTest, formIdRefIdHasStrongOrderWithStringView)
        {
            const RefId formIdRefId = RefId::formIdRefId({ .mIndex = 42, .mContentFile = 0 });
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

        TEST(ESMRefIdTest, emptyRefId)
        {
            EXPECT_EQ(RefId(), EmptyRefId());
            EXPECT_EQ(RefId(), RefId::stringRefId("\0"));
            EXPECT_EQ(RefId(), RefId::formIdRefId({ .mIndex = 0, .mContentFile = 0 }));
            EXPECT_EQ(RefId(), RefId::formIdRefId({ .mIndex = 0, .mContentFile = -1 }));
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

        TEST(ESMRefIdTest, esm3ExteriorCellHasLexicographicalOrder)
        {
            const RefId a = RefId::esm3ExteriorCell(0, 0);
            const RefId b = RefId::esm3ExteriorCell(1, 0);
            EXPECT_LT(a, b);
            EXPECT_TRUE(!(b < a));
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
            { RefId::formIdRefId({ .mIndex = 42, .mContentFile = 0 }), "0x2a" },
            { RefId::formIdRefId({ .mIndex = 0xffffff, .mContentFile = std::numeric_limits<std::int32_t>::max() }),
                "0x7fffffffffffff" },
            { RefId::generated(42), "0x2a" },
            { RefId::generated(std::numeric_limits<std::uint64_t>::max()), "0xffffffffffffffff" },
            { RefId::index(REC_ARMO, 42), "ARMO:0x2a" },
            { RefId::esm3ExteriorCell(-13, 42), "#-13 42" },
            { RefId::esm3ExteriorCell(std::numeric_limits<int>::min(), std::numeric_limits<int>::min()),
                "#-2147483648 -2147483648" },
            { RefId::esm3ExteriorCell(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()),
                "#2147483647 2147483647" },
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
            { RefId::stringRefId(std::string({ 'a', 0, -1, '\n', '\t' })), "\"a\\x0\\xff\\xa\\x9\"" },
            { RefId::stringRefId("Логово дракона"), "\"Логово дракона\"" },
            { RefId::stringRefId("\xd0\x9b"), "\"Л\"" },
            { RefId::stringRefId("\xff\x9b"), "\"\\xff\\x9b\"" },
            { RefId::stringRefId("\xd0\xd0"), "\"\\xd0\\xd0\"" },
            { RefId::formIdRefId({ .mIndex = 42, .mContentFile = 0 }), "FormId:0x2a" },
            { RefId::formIdRefId({ .mIndex = 0xffffff, .mContentFile = std::numeric_limits<std::int32_t>::max() }),
                "FormId:0x7fffffffffffff" },
            { RefId::generated(42), "Generated:0x2a" },
            { RefId::generated(std::numeric_limits<std::uint64_t>::max()), "Generated:0xffffffffffffffff" },
            { RefId::index(REC_ARMO, 42), "Index:ARMO:0x2a" },
            { RefId::index(REC_ARMO, std::numeric_limits<std::uint32_t>::max()), "Index:ARMO:0xffffffff" },
            { RefId::esm3ExteriorCell(-13, 42), "Esm3ExteriorCell:-13:42" },
            { RefId::esm3ExteriorCell(std::numeric_limits<int>::min(), std::numeric_limits<int>::min()),
                "Esm3ExteriorCell:-2147483648:-2147483648" },
            { RefId::esm3ExteriorCell(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()),
                "Esm3ExteriorCell:2147483647:2147483647" },
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
            { RefId::formIdRefId({ .mIndex = 1, .mContentFile = 0 }), "FormId:0x1" },
            { RefId::formIdRefId({ .mIndex = 0x1f, .mContentFile = 0 }), "FormId:0x1f" },
            { RefId::formIdRefId({ .mIndex = 0x1f, .mContentFile = 2 }), "FormId:0x200001f" },
            { RefId::formIdRefId({ .mIndex = 0xffffff, .mContentFile = 0x1abc }), "FormId:0x1abcffffff" },
            { RefId::formIdRefId({ .mIndex = 0xffffff, .mContentFile = std::numeric_limits<std::int32_t>::max() }),
                "FormId:0x7fffffffffffff" },
            { RefId::generated(0), "Generated:0x0" },
            { RefId::generated(1), "Generated:0x1" },
            { RefId::generated(0x1f), "Generated:0x1f" },
            { RefId::generated(std::numeric_limits<std::uint64_t>::max()), "Generated:0xffffffffffffffff" },
            { RefId::index(REC_INGR, 0), "Index:INGR:0x0" },
            { RefId::index(REC_INGR, 1), "Index:INGR:0x1" },
            { RefId::index(REC_INGR, 0x1f), "Index:INGR:0x1f" },
            { RefId::index(REC_INGR, std::numeric_limits<std::uint32_t>::max()), "Index:INGR:0xffffffff" },
            { RefId::esm3ExteriorCell(-13, 42), "Esm3ExteriorCell:-13:42" },
            { RefId::esm3ExteriorCell(
                  std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::min()),
                "Esm3ExteriorCell:-2147483648:-2147483648" },
            { RefId::esm3ExteriorCell(
                  std::numeric_limits<std::int32_t>::max(), std::numeric_limits<std::int32_t>::max()),
                "Esm3ExteriorCell:2147483647:2147483647" },
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
        struct GenerateRefId<FormId>
        {
            static RefId call() { return FormId{ .mIndex = 42, .mContentFile = 0 }; }
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
            const std::string text = refId.serializeText();
            EXPECT_EQ(RefId::deserializeText(text), refId);
        }

        TYPED_TEST_P(ESMRefIdTypesTest, serializeTextShouldReturnOnlyPrintableCharacters)
        {
            const RefId refId = GenerateRefId<TypeParam>::call();
            EXPECT_THAT(refId.serializeText(), Each(IsPrint()));
        }

        TYPED_TEST_P(ESMRefIdTypesTest, toStringShouldReturnOnlyPrintableCharacters)
        {
            const RefId refId = GenerateRefId<TypeParam>::call();
            EXPECT_THAT(refId.toString(), Each(IsPrint()));
        }

        TYPED_TEST_P(ESMRefIdTypesTest, toDebugStringShouldReturnOnlyPrintableCharacters)
        {
            const RefId refId = GenerateRefId<TypeParam>::call();
            EXPECT_THAT(refId.toDebugString(), Each(IsPrint()));
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

        TYPED_TEST_P(ESMRefIdTypesTest, saveAndLoadShouldNotChange)
        {
            constexpr NAME fakeRecordId(fourCC("FAKE"));
            constexpr NAME subRecordId(fourCC("NAME"));
            const RefId expected = GenerateRefId<TypeParam>::call();
            auto stream = std::make_unique<std::stringstream>();
            {
                ESMWriter writer;
                writer.setFormatVersion(CurrentSaveGameFormatVersion);
                writer.save(*stream);
                writer.startRecord(fakeRecordId);
                writer.writeHNCRefId(subRecordId, expected);
                writer.endRecord(fakeRecordId);
            }
            ESMReader reader;
            reader.open(std::move(stream), "stream");
            ASSERT_TRUE(reader.hasMoreRecs());
            ASSERT_EQ(reader.getRecName().toInt(), fakeRecordId);
            reader.getRecHeader();
            const RefId actual = reader.getHNRefId(subRecordId);
            EXPECT_EQ(actual, expected);
        }

        REGISTER_TYPED_TEST_SUITE_P(ESMRefIdTypesTest, serializeThenDeserializeShouldProduceSameValue,
            serializeTextThenDeserializeTextShouldProduceSameValue, shouldBeEqualToItself, shouldNotBeNotEqualToItself,
            shouldBeNotLessThanItself, serializeTextShouldReturnOnlyPrintableCharacters,
            toStringShouldReturnOnlyPrintableCharacters, toDebugStringShouldReturnOnlyPrintableCharacters,
            saveAndLoadShouldNotChange);

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
