#include "apps/opencs/model/world/universalid.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <stdexcept>

namespace CSMWorld
{
    namespace
    {
        using namespace ::testing;

        TEST(CSMWorldUniversalIdTest, shouldFailToConstructFromNoneWithInvalidType)
        {
            EXPECT_THROW(
                UniversalId{ static_cast<UniversalId::Type>(std::numeric_limits<int>::max()) }, std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, shouldFailToConstructFromStringWithInvalidType)
        {
            EXPECT_THROW(UniversalId(UniversalId::Type_Search, "invalid"), std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, shouldFailToConstructFromIntWithInvalidType)
        {
            EXPECT_THROW(UniversalId(UniversalId::Type_Activator, 42), std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, shouldFailToConstructFromRefIdWithInvalidType)
        {
            EXPECT_THROW(UniversalId(UniversalId::Type_Search, ESM::RefId()), std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, shouldFailToConstructFromInvalidUniversalIdString)
        {
            EXPECT_THROW(UniversalId("invalid"), std::runtime_error);
        }

        TEST(CSMWorldUniversalIdTest, getIndexShouldThrowExceptionForDefaultConstructed)
        {
            const UniversalId id;
            EXPECT_THROW(id.getIndex(), std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, getIndexShouldThrowExceptionForConstructedFromString)
        {
            const UniversalId id(UniversalId::Type_Activator, "a");
            EXPECT_THROW(id.getIndex(), std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, getIndexShouldReturnValueForConstructedFromInt)
        {
            const UniversalId id(UniversalId::Type_Search, 42);
            EXPECT_EQ(id.getIndex(), 42);
        }

        TEST(CSMWorldUniversalIdTest, getIdShouldThrowExceptionForConstructedFromInt)
        {
            const UniversalId id(UniversalId::Type_Search, 42);
            EXPECT_THROW(id.getId(), std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, getIdShouldReturnValueForConstructedFromString)
        {
            const UniversalId id(UniversalId::Type_Activator, "a");
            EXPECT_EQ(id.getId(), "a");
        }

        TEST(CSMWorldUniversalIdTest, getRefIdShouldThrowExceptionForDefaultConstructed)
        {
            const UniversalId id;
            EXPECT_THROW(id.getRefId(), std::logic_error);
        }

        TEST(CSMWorldUniversalIdTest, getRefIdShouldReturnValueForConstructedFromRefId)
        {
            const UniversalId id(UniversalId::Type_Skill, ESM::IndexRefId(ESM::REC_SKIL, 42));
            EXPECT_EQ(id.getRefId(), ESM::IndexRefId(ESM::REC_SKIL, 42));
        }

        struct Params
        {
            UniversalId mId;
            UniversalId::Type mType;
            UniversalId::Class mClass;
            UniversalId::ArgumentType mArgumentType;
            std::string mTypeName;
            std::string mString;
            std::string mIcon;
        };

        std::ostream& operator<<(std::ostream& stream, const Params& value)
        {
            return stream << ".mType = " << value.mType << " .mClass = " << value.mClass
                          << " .mArgumentType = " << value.mArgumentType << " .mTypeName = " << value.mTypeName
                          << " .mString = " << value.mString << " .mIcon = " << value.mIcon;
        }

        struct CSMWorldUniversalIdValidPerTypeTest : TestWithParam<Params>
        {
        };

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, getTypeShouldReturnExpected)
        {
            EXPECT_EQ(GetParam().mId.getType(), GetParam().mType);
        }

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, getClassShouldReturnExpected)
        {
            EXPECT_EQ(GetParam().mId.getClass(), GetParam().mClass);
        }

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, getArgumentTypeShouldReturnExpected)
        {
            EXPECT_EQ(GetParam().mId.getArgumentType(), GetParam().mArgumentType);
        }

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, shouldBeEqualToCopy)
        {
            EXPECT_EQ(GetParam().mId, UniversalId(GetParam().mId));
        }

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, shouldNotBeLessThanCopy)
        {
            EXPECT_FALSE(GetParam().mId < UniversalId(GetParam().mId));
        }

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, getTypeNameShouldReturnExpected)
        {
            EXPECT_EQ(GetParam().mId.getTypeName(), GetParam().mTypeName);
        }

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, toStringShouldReturnExpected)
        {
            EXPECT_EQ(GetParam().mId.toString(), GetParam().mString);
        }

        TEST_P(CSMWorldUniversalIdValidPerTypeTest, getIconShouldReturnExpected)
        {
            EXPECT_EQ(GetParam().mId.getIcon(), GetParam().mIcon);
        }

        const std::array validParams = {
            Params{ UniversalId(), UniversalId::Type_None, UniversalId::Class_None, UniversalId::ArgumentType_None, "-",
                "-", ":placeholder" },

            Params{ UniversalId(UniversalId::Type_None), UniversalId::Type_None, UniversalId::Class_None,
                UniversalId::ArgumentType_None, "-", "-", ":placeholder" },
            Params{ UniversalId(UniversalId::Type_RegionMap), UniversalId::Type_RegionMap, UniversalId::Class_NonRecord,
                UniversalId::ArgumentType_None, "Region Map", "Region Map", ":region-map" },
            Params{ UniversalId(UniversalId::Type_RunLog), UniversalId::Type_RunLog, UniversalId::Class_Transient,
                UniversalId::ArgumentType_None, "Run Log", "Run Log", ":run-log" },
            Params{ UniversalId(UniversalId::Type_Lands), UniversalId::Type_Lands, UniversalId::Class_RecordList,
                UniversalId::ArgumentType_None, "Lands", "Lands", ":land-heightmap" },
            Params{ UniversalId(UniversalId::Type_Icons), UniversalId::Type_Icons, UniversalId::Class_ResourceList,
                UniversalId::ArgumentType_None, "Icons", "Icons", ":resources-icon" },

            Params{ UniversalId(UniversalId::Type_Activator, "a"), UniversalId::Type_Activator,
                UniversalId::Class_RefRecord, UniversalId::ArgumentType_Id, "Activator", "Activator: a", ":activator" },
            Params{ UniversalId(UniversalId::Type_Gmst, "b"), UniversalId::Type_Gmst, UniversalId::Class_Record,
                UniversalId::ArgumentType_Id, "Game Setting", "Game Setting: b", ":gmst" },
            Params{ UniversalId(UniversalId::Type_Mesh, "c"), UniversalId::Type_Mesh, UniversalId::Class_Resource,
                UniversalId::ArgumentType_Id, "Mesh", "Mesh: c", ":resources-mesh" },
            Params{ UniversalId(UniversalId::Type_Scene, "d"), UniversalId::Type_Scene, UniversalId::Class_Collection,
                UniversalId::ArgumentType_Id, "Scene", "Scene: d", ":scene" },
            Params{ UniversalId(UniversalId::Type_Reference, "e"), UniversalId::Type_Reference,
                UniversalId::Class_SubRecord, UniversalId::ArgumentType_Id, "Instance", "Instance: e", ":instance" },

            Params{ UniversalId(UniversalId::Type_Search, 42), UniversalId::Type_Search, UniversalId::Class_Transient,
                UniversalId::ArgumentType_Index, "Global Search", "Global Search: 42", ":menu-search" },

            Params{ UniversalId("Instance: f"), UniversalId::Type_Reference, UniversalId::Class_SubRecord,
                UniversalId::ArgumentType_Id, "Instance", "Instance: f", ":instance" },

            Params{ UniversalId(UniversalId::Type_Reference, ESM::RefId::stringRefId("g")), UniversalId::Type_Reference,
                UniversalId::Class_SubRecord, UniversalId::ArgumentType_RefId, "Instance", "Instance: g", ":instance" },
            Params{ UniversalId(UniversalId::Type_Reference, ESM::RefId::index(ESM::REC_SKIL, 42)),
                UniversalId::Type_Reference, UniversalId::Class_SubRecord, UniversalId::ArgumentType_RefId, "Instance",
                "Instance: SKIL:0x2a", ":instance" },
        };

        INSTANTIATE_TEST_SUITE_P(ValidParams, CSMWorldUniversalIdValidPerTypeTest, ValuesIn(validParams));
    }
}
