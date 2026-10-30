#ifndef COMPONENT_ESM_COMMON_H
#define COMPONENT_ESM_COMMON_H

#include <algorithm>
#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <components/misc/strings/algorithm.hpp>

namespace ESM
{
#pragma pack(push, 1)
    union ESMVersion
    {
        float f;
        std::uint32_t ui;
    };

    union TypeId
    {
        std::uint32_t value;
        char name[4]; // record type in ascii
    };
#pragma pack(pop)

    enum ESMVersions
    {
        VER_120 = 0x3f99999a, // TES3
        VER_130 = 0x3fa66666, // TES3
        VER_080 = 0x3f4ccccd, // TES4
        VER_100 = 0x3f800000, // TES4, FO4
        VER_132 = 0x3fa8f5c3, // FONV Courier's Stash, DeadMoney
        VER_133 = 0x3faa3d71, // FONV HonestHearts
        VER_134 = 0x3fab851f, // FONV, GunRunnersArsenal, LonesomeRoad, OldWorldBlues
        VER_094 = 0x3f70a3d7, // TES5/FO3
        VER_170 = 0x3fd9999a, // TES5
        VER_171 = 0x3fdae148, // TES5
        VER_095 = 0x3f733333, // FO4
    };

    // Defines another files (esm or esp) that this file depends upon.
    struct MasterData
    {
        std::string name;
        std::uint64_t size;
    };

    enum class GameFormat
    {
        Unknown,
        TES3,
        TES4,
        FO3,
        FONV,
        TES5,
        FO4,
    };

    inline constexpr std::pair<std::string_view, GameFormat> gameFormatNames[] = {
        { "TES3", GameFormat::TES3 },
        { "TES4", GameFormat::TES4 },
        { "FO3", GameFormat::FO3 },
        { "FONV", GameFormat::FONV },
        { "TES5", GameFormat::TES5 },
        { "FO4", GameFormat::FO4 },
    };

    inline std::istream& operator>>(std::istream& stream, GameFormat& format)
    {
        std::string name;
        stream >> name;
        const auto it = std::ranges::find_if(
            gameFormatNames, [&](const auto& entry) { return Misc::StringUtils::ciEqual(entry.first, name); });
        if (it == std::end(gameFormatNames))
            stream.setstate(std::ios::failbit);
        else
            format = it->second;
        return stream;
    }

    inline std::ostream& operator<<(std::ostream& stream, GameFormat format)
    {
        const auto it = std::ranges::find(gameFormatNames, format, &std::pair<std::string_view, GameFormat>::second);
        return stream << (it != std::end(gameFormatNames) ? it->first : "Unknown");
    }

    // The game format associated with a list of masters.
    inline std::optional<GameFormat> getGameFormatFromMasters(
        const std::vector<MasterData>& masters, std::span<const GameFormat> masterFormats = {})
    {
        static constexpr std::pair<std::string_view, GameFormat> ambiguousMainFiles[] = {
            { "Fallout3.esm", GameFormat::FO3 },
            { "Skyrim.esm", GameFormat::TES5 },
        };

        // Search through previously-loaded masters
        for (GameFormat format : masterFormats)
            if (format != GameFormat::Unknown)
                return format;

        // Otherwise, find a matching name. Still useful for the ESM tool, which doesn't load masters.
        for (const MasterData& master : masters)
            for (const auto& [name, format] : ambiguousMainFiles)
                if (Misc::StringUtils::ciEqual(master.name, name))
                    return format;
        return std::nullopt;
    }

    // The game format for a content file. Games weren't meant to coexist, so it's our best educated guess.
    // ESM version -> previous-masters' game format -> matching master names -> header form version
    inline GameFormat getGameFormat(std::uint32_t version, std::optional<std::uint16_t> headerFormVersion,
        const std::vector<MasterData>& masters, std::span<const GameFormat> masterFormats = {})
    {
        switch (version)
        {
            case VER_120:
            case VER_130:
                return GameFormat::TES3;
            case VER_080:
                return GameFormat::TES4;
            case VER_100:
                return headerFormVersion.has_value() ? GameFormat::FO4 : GameFormat::TES4;
            case VER_094:
                if (const std::optional<GameFormat> format = getGameFormatFromMasters(masters, masterFormats))
                    return *format;
                return headerFormVersion.value_or(0) <= 15 ? GameFormat::FO3 : GameFormat::TES5;
            case VER_132:
            case VER_133:
            case VER_134:
                return GameFormat::FONV;
            case VER_170:
            case VER_171:
                return GameFormat::TES5;
            case VER_095:
                return GameFormat::FO4;
        }
        return GameFormat::Unknown;
    }

    inline std::string printName(const std::uint32_t typeId)
    {
        unsigned char typeName[4];
        typeName[0] = typeId & 0xff;
        typeName[1] = (typeId >> 8) & 0xff;
        typeName[2] = (typeId >> 16) & 0xff;
        typeName[3] = (typeId >> 24) & 0xff;

        return std::string(reinterpret_cast<char*>(typeName), 4);
    }
}

#endif // COMPONENT_ESM_COMMON_H
