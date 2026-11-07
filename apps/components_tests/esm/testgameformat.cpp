#include <components/esm/gameformat.hpp>

#include <gtest/gtest.h>

#include <initializer_list>
#include <optional>
#include <sstream>
#include <vector>

namespace
{
    using ESM::GameFormat;

    std::vector<ESM::MasterData> masters(std::initializer_list<const char*> names)
    {
        std::vector<ESM::MasterData> result;
        for (const char* name : names)
            result.push_back({ name, 0 });
        return result;
    }

    // Header values are from vanilla masters and mods
    TEST(EsmGameFormat, shouldDetectUnambiguousVersions)
    {
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_130, std::nullopt), GameFormat::TES3);
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_080, std::nullopt), GameFormat::TES4);
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_100, std::nullopt), GameFormat::TES4);
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_134, 2), GameFormat::FNV);
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_171, 44, masters({ "Skyrim.esm" })), GameFormat::TES5);
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_100, 131), GameFormat::FO4);
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_095, 131, masters({ "Fallout4.esm" })), GameFormat::FO4);
    }

    TEST(EsmGameFormat, shouldReturnUnknownForUnsupportedVersions)
    {
        constexpr std::uint32_t unk = 0xffffffff;
        EXPECT_EQ(ESM::getGameFormat(unk, 582, masters({ "unk.esm" })), GameFormat::Unknown);
    }

    TEST(EsmGameFormat, shouldUseFormVersionForMasterlessVersion094)
    {
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 2), GameFormat::FO3); // Fallout3.esm
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 40), GameFormat::TES5); // Skyrim.esm
    }

    TEST(EsmGameFormat, shouldUseMasterNamesForVersion094)
    {
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 40, masters({ "Fallout3.esm" })), GameFormat::FO3);
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 15, masters({ "Skyrim.esm" })), GameFormat::TES5);
    }

    TEST(EsmGameFormat, shouldUseMainFileAfterOtherMastersForVersion094)
    {
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 2, masters({ "SomeMod.esp", "Skyrim.esm" })), GameFormat::TES5);
    }

    TEST(EsmGameFormat, shouldPreferMasterFormatsToMasterNamesForVersion094)
    {
        const std::vector<GameFormat> tes5 = { GameFormat::TES5 }; // Skyrim mod
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 15, masters({ "SomeMod.esp" }), tes5), GameFormat::TES5);

        const std::vector<GameFormat> fo3 = { GameFormat::FO3 }; // Fallout 3 file named Skyrim.esm
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 40, masters({ "Skyrim.esm" }), fo3), GameFormat::FO3);
    }

    TEST(EsmGameFormat, shouldUseFirstKnownMasterFormatForVersion094)
    {
        const std::vector<GameFormat> formats = { GameFormat::Unknown, GameFormat::TES5, GameFormat::FO3 };
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_094, 15, masters({ "Missing.esp", "A.esm", "B.esm" }), formats),
            GameFormat::TES5);
    }

    TEST(EsmGameFormat, shouldIgnoreMasterFormatsForUnambiguousVersions)
    {
        const std::vector<GameFormat> fo3 = { GameFormat::FO3 };
        EXPECT_EQ(ESM::getGameFormat(ESM::VER_171, 44, masters({ "Skyrim.esm" }), fo3), GameFormat::TES5);
    }

    TEST(EsmGameFormat, shouldReadNameIgnoringCase)
    {
        GameFormat format = GameFormat::Unknown;
        std::istringstream stream("fo3");
        EXPECT_TRUE(stream >> format);
        EXPECT_EQ(format, GameFormat::FO3);
    }

    TEST(EsmGameFormat, shouldFailToReadUnknownName)
    {
        GameFormat format = GameFormat::TES5;
        std::istringstream stream("FONV");
        EXPECT_FALSE(stream >> format);
        EXPECT_EQ(format, GameFormat::TES5);
    }

    TEST(EsmGameFormat, shouldWriteName)
    {
        std::ostringstream stream;
        stream << GameFormat::FNV << ' ' << GameFormat::Unknown;
        EXPECT_EQ(stream.str(), "FNV Unknown");
    }

    TEST(EsmGameFormat, getGameFormatFromMastersShouldReturnNulloptWithNoValidMaster)
    {
        const std::vector<GameFormat> unknown = { GameFormat::Unknown };
        EXPECT_EQ(ESM::getGameFormatFromMasters(masters({ "SomeMod.esp" }), unknown), std::nullopt);
        EXPECT_EQ(ESM::getGameFormatFromMasters({}), std::nullopt);
    }
}
